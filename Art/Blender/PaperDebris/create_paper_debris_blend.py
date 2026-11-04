import math
import random
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh

# RulesHorror paper debris generator: loose, slightly bent A4 sheets lying on the floor.
# Replaces the purchased BrutalistLevelKit SM_Debris_Paper_Set1 / Set5 / Set6.
# Each mesh keeps the original bounds and pivot (floor level, pile center) so placed actors keep their transform.
# Every sheet is a thin closed slab (0.3 mm), so no two-sided material is needed.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "PaperDebris.blend"
EXPORT_DIR = BASE_DIR / "Exports"
TEXTURE_PATH = BASE_DIR / "Textures" / "T_PaperDebris_Printed.png"

MAT_PAPER = "Paper_Printed"

A4 = (0.210, 0.297)       # sheet width (local s) x length (local t), meters
THICK = 0.0003            # sheet thickness
GAP = 0.0016              # clearance between overlapping sheets
DRAPE_SLOPE = 0.04        # how fast an upper sheet falls off the edge of a lower one
NU, NV = 7, 10            # grid resolution per sheet

# Texture atlas (1024 x 1024): two printed A4 pages side by side on top, blank paper strip below.
TEX_SIZE = 1024
PAGE_V0 = 1.0 - 0.707     # pages occupy v in [PAGE_V0, 1]; u in [0, 0.5] and [0.5, 1]
BLANK_UV = (0.03, 0.03, 0.97, PAGE_V0 - 0.03)  # u0, v0, u1, v1 of the blank strip


def ue_to_blender_bounds(ue_min, ue_max):
    """Unreal cm bounds -> Blender meter bounds (Blender Y = -Unreal Y)."""
    lo = (ue_min[0] / 100.0, -ue_max[1] / 100.0, ue_min[2] / 100.0)
    hi = (ue_max[0] / 100.0, -ue_min[1] / 100.0, ue_max[2] / 100.0)
    return lo, hi


# Sheet layouts. c = rough center (Blender m, refined by fit_layout), rot = degrees about Z,
# page = 0/1 printed page facing up, None = blank back side facing up, flip = print rotated 180 degrees.
# Deformations (a = across width, b = along length, both -1..1):
#   ("arch_a"/"arch_b", amp)            middle raised, ends on the floor
#   ("bend_a"/"bend_b", amp)            ends raised, middle on the floor
#   ("curl", "a+"|"a-"|"b+"|"b-", start, amp, power)   one edge curls up
#   ("corner", ca, cb, start, amp)      one corner lifts (ca, cb = +-1)
#   ("fold", dx, dy, offset_m, slope)   flap bent up along a straight crease (direction in local meters)
#   ("wave", amp, fa, fb, phase)        soft wrinkle
PROPS = {
    "SM_PaperDebris_A": dict(
        replaces="/Game/Assets/BrutalistLevelKit/Models/Props/Debris/SM_Debris_Paper_Set1",
        ue_min=(-34.8, -24.8, -0.2), ue_max=(37.2, 32.2, 5.0),
        sheets=[
            dict(c=(-0.20, 0.06), rot=14, page=0, deform=[("bend_a", 0.004), ("curl", "b+", 0.6, 0.012, 2.0),
                                                           ("wave", 0.0015, 1.3, 0.7, 0.4)]),
            dict(c=(0.15, 0.12), rot=-72, page=1, deform=[("corner", 1, 1, 0.55, 0.018), ("wave", 0.002, 0.8, 1.6, 1.1)]),
            dict(c=(0.22, -0.13), rot=-18, page=None, deform=[("fold", 0.8, -0.6, 0.06, 0.32),
                                                              ("wave", 0.0015, 1.1, 1.2, 2.0)]),
            dict(c=(-0.24, -0.16), rot=-8, page=1, flip=True, deform=[("curl", "a-", 0.4, 0.010, 2.0),
                                                                     ("wave", 0.002, 1.5, 0.9, 0.3)]),
            dict(c=(-0.05, -0.08), rot=48, page=0, flip=True, deform=[("arch_b", 0.045), ("bend_a", 0.003)]),
            dict(c=(0.04, 0.02), rot=98, page=1, deform=[("curl", "b-", 0.5, 0.008, 2.0), ("wave", 0.002, 1.2, 1.4, 0.8)]),
        ]),
    "SM_PaperDebris_B": dict(
        replaces="/Game/Assets/BrutalistLevelKit/Models/Props/Debris/SM_Debris_Paper_Set5",
        ue_min=(-33.5, -17.9, 0.2), ue_max=(29.4, 29.6, 2.3),
        sheets=[
            dict(c=(-0.24, 0.0), rot=12, page=1, deform=[("curl", "b-", 0.5, 0.010, 2.0), ("wave", 0.002, 1.0, 1.3, 0.2)]),
            dict(c=(0.14, -0.08), rot=-28, page=0, deform=[("bend_b", 0.004), ("wave", 0.0015, 1.4, 0.8, 1.5)]),
            dict(c=(0.13, 0.03), rot=62, page=None, deform=[("fold", -0.7, 0.7, 0.03, 0.13), ("wave", 0.0015, 0.9, 1.1, 0.6)]),
        ]),
    "SM_PaperDebris_C": dict(
        replaces="/Game/Assets/BrutalistLevelKit/Models/Props/Debris/SM_Debris_Paper_Set6",
        ue_min=(-17.9, -19.8, 0.0), ue_max=(32.6, 26.1, 3.2),
        sheets=[
            dict(c=(-0.05, 0.02), rot=4, page=0, deform=[("curl", "a-", 0.35, 0.026, 2.2), ("wave", 0.002, 1.1, 0.9, 0.5)]),
            dict(c=(0.14, -0.04), rot=-40, page=1, deform=[("fold", 0.105, 0.1485, 0.10, 0.25), ("bend_a", 0.003)]),
            dict(c=(0.06, 0.09), rot=28, page=0, flip=True, deform=[("corner", -1, 1, 0.5, 0.012),
                                                                   ("wave", 0.002, 1.3, 1.0, 2.2)]),
        ]),
}


# -----------------------------------------------------------------------------
# Texture
# -----------------------------------------------------------------------------


def smooth_noise(h, w, cells, rng):
    """Bilinear-upsampled random grid, values 0..1."""
    gh, gw = cells + 1, max(2, int(cells * w / h) + 1)
    g = rng.random((gh, gw))
    ys = np.linspace(0, gh - 1, h)
    xs = np.linspace(0, gw - 1, w)
    y0 = np.minimum(ys.astype(int), gh - 2)
    x0 = np.minimum(xs.astype(int), gw - 2)
    fy = (ys - y0)[:, None]
    fx = (xs - x0)[None, :]
    a = g[y0][:, x0]
    b = g[y0][:, x0 + 1]
    c = g[y0 + 1][:, x0]
    d = g[y0 + 1][:, x0 + 1]
    return a * (1 - fx) * (1 - fy) + b * fx * (1 - fy) + c * (1 - fx) * fy + d * fx * fy


def ink(img, x0, y0, x1, y1, tone):
    """Darken a rectangle (pixel coords, top-left origin) toward ink tone 0..1 (sRGB)."""
    h, w = img.shape[:2]
    x0, x1 = max(0, int(x0)), min(w, int(x1))
    y0, y1 = max(0, int(y0)), min(h, int(y1))
    if x1 > x0 and y1 > y0:
        img[y0:y1, x0:x1, :3] = np.minimum(img[y0:y1, x0:x1, :3], tone)


def text_line(img, rng, x0, x1, y, height, tone, ragged=False):
    """A row of grey dashes suggesting words (no readable text)."""
    x = x0
    end = x1 - (rng.uniform(0.2, 0.6) * (x1 - x0) if ragged else 0)
    while x < end:
        wlen = rng.integers(8, 38)
        ink(img, x, y, min(x + wlen, end), y + height, tone)
        x += wlen + rng.integers(4, 8)


def paragraph(img, rng, x0, x1, y, lines, pitch=12, height=4, tone=0.45):
    for i in range(lines):
        text_line(img, rng, x0, x1, y + i * pitch, height, tone, ragged=(i == lines - 1))
    return y + lines * pitch


def table(img, rng, x0, y0, cols, rows, cw, rh_px, tone=0.5):
    w, h = cols * cw, rows * rh_px
    for r in range(rows + 1):
        ink(img, x0, y0 + r * rh_px, x0 + w, y0 + r * rh_px + 1, tone)
    for c in range(cols + 1):
        ink(img, x0 + c * cw, y0, x0 + c * cw + 1, y0 + h, tone)
    ink(img, x0, y0, x0 + w, y0 + rh_px, 0.78)  # shaded header row
    for r in range(rows):
        for c in range(cols):
            ln = rng.integers(int(cw * 0.25), int(cw * 0.7))
            ink(img, x0 + c * cw + 6, y0 + r * rh_px + rh_px // 2 - 2, x0 + c * cw + 6 + ln,
                y0 + r * rh_px + rh_px // 2 + 2, 0.32 if r == 0 else 0.5)
    return y0 + h


def draw_page(img, rng, variant):
    """Printed A4 page into img (h x w x 4, top-left origin)."""
    h, w = img.shape[:2]
    ml, mr = 42, w - 42
    y = 44
    if variant == 0:
        # Report: title bar, rule, paragraphs, a table, more text, signature line.
        ink(img, ml, y, ml + 190, y + 14, 0.22)
        ink(img, mr - 70, y + 2, mr, y + 8, 0.5)
        y += 24
        ink(img, ml, y, mr, y + 2, 0.35)
        y += 16
        y = paragraph(img, rng, ml, mr, y, 6) + 10
        y = paragraph(img, rng, ml, mr, y, 4) + 16
        ink(img, ml, y, ml + 120, y + 8, 0.3)
        y += 18
        y = table(img, rng, ml, y, 4, 7, (mr - ml) // 4, 20) + 18
        y = paragraph(img, rng, ml, mr, y, 5) + 10
        y = paragraph(img, rng, ml, mr, y, 3) + 30
        ink(img, ml, y, ml + 140, y + 1, 0.35)
        ink(img, mr - 140, y, mr, y + 1, 0.35)
        ink(img, ml, y + 8, ml + 70, y + 12, 0.55)
        ink(img, mr - 140, y + 8, mr - 80, y + 12, 0.55)
    else:
        # Form: letterhead box, field rows with underlines, checkboxes, big table, footer.
        ink(img, ml, y, ml + 46, y + 46, 0.4)
        ink(img, ml + 8, y + 8, ml + 38, y + 38, 0.88)
        ink(img, ml + 60, y + 6, ml + 230, y + 18, 0.25)
        ink(img, ml + 60, y + 26, ml + 180, y + 31, 0.5)
        ink(img, mr - 90, y + 6, mr, y + 40, 0.55)
        ink(img, mr - 86, y + 10, mr - 4, y + 36, 0.92)
        y += 66
        ink(img, ml, y, mr, y + 3, 0.3)
        y += 18
        for i in range(6):
            lab = rng.integers(40, 80)
            ink(img, ml, y + 4, ml + lab, y + 9, 0.4)
            ink(img, ml + 95, y + 14, mr, y + 15, 0.55)
            if rng.random() < 0.7:
                text_line(img, rng, ml + 100, ml + 100 + rng.integers(60, 220), y + 5, 4, 0.5)
            y += 24
        y += 6
        for i in range(3):
            bx = ml + i * 140
            ink(img, bx, y, bx + 12, y + 12, 0.35)
            ink(img, bx + 2, y + 2, bx + 10, y + 10, 0.93)
            if i == 1:
                ink(img, bx + 3, y + 3, bx + 9, y + 9, 0.3)
            ink(img, bx + 20, y + 4, bx + 20 + rng.integers(50, 90), y + 9, 0.45)
        y += 30
        y = table(img, rng, ml, y, 5, 9, (mr - ml) // 5, 22) + 20
        y = paragraph(img, rng, ml, mr, y, 4, tone=0.5) + 20
        ink(img, ml, h - 60, mr, h - 59, 0.5)
        text_line(img, rng, ml, ml + 260, h - 52, 3, 0.6)


def wear(img, rng, strength=1.0):
    """Dirt, slight yellowing and darker edges on a page region."""
    h, w = img.shape[:2]
    n1 = smooth_noise(h, w, 6, rng)
    n2 = smooth_noise(h, w, 22, rng)
    dirt = 1.0 - strength * (0.07 * n1 + 0.04 * n2)
    yy, xx = np.mgrid[0:h, 0:w]
    edge = np.minimum(np.minimum(xx, w - 1 - xx), np.minimum(yy, h - 1 - yy)).astype(float)
    dirt *= 1.0 - strength * 0.08 * np.clip(1.0 - edge / 25.0, 0.0, 1.0)
    img[:, :, 0] *= dirt
    img[:, :, 1] *= dirt * 0.995
    img[:, :, 2] *= dirt * 0.975
    # A faint dusty footprint-like smudge / water mark.
    cy, cx = rng.uniform(0.3, 0.8) * h, rng.uniform(0.2, 0.8) * w
    r = rng.uniform(0.12, 0.2) * w
    d = np.hypot(yy - cy, xx - cx) / r
    ring = np.exp(-((d - 1.0) ** 2) / 0.004) * 0.06 + np.clip(1.0 - d, 0, 1) * 0.03
    img[:, :, :3] *= (1.0 - strength * ring)[:, :, None]


def build_texture():
    rng = np.random.default_rng(7)
    S = TEX_SIZE
    img = np.ones((S, S, 4), dtype=np.float64)
    paper = np.array([0.94, 0.935, 0.90])
    img[:, :, :3] = paper
    fibre = smooth_noise(S, S, 140, rng) * 0.025 + rng.random((S, S)) * 0.012
    img[:, :, :3] -= fibre[:, :, None]
    page_h = int(round(S * (1.0 - PAGE_V0)))
    for p in range(2):
        sub = img[0:page_h, p * S // 2:(p + 1) * S // 2]
        draw_page(sub, rng, p)
        wear(sub, rng, 1.0)
    wear(img[page_h:, :], rng, 0.7)
    img = np.clip(img, 0.0, 1.0)
    image = bpy.data.images.new("T_PaperDebris_Printed", S, S, alpha=False)
    image.pixels.foreach_set(np.flipud(img).astype(np.float32).ravel())
    rh.save_image(image, TEXTURE_PATH)
    return image


# -----------------------------------------------------------------------------
# Sheets
# -----------------------------------------------------------------------------


def sheet_height(sheet, s, t):
    w, l = sheet["size"]
    a, b = 2.0 * s / w, 2.0 * t / l
    h = 0.0
    for d in sheet["deform"]:
        kind = d[0]
        if kind == "arch_a":
            h += d[1] * (1.0 - a * a)
        elif kind == "arch_b":
            h += d[1] * (1.0 - b * b)
        elif kind == "bend_a":
            h += d[1] * a * a
        elif kind == "bend_b":
            h += d[1] * b * b
        elif kind == "curl":
            x = {"a+": a, "a-": -a, "b+": b, "b-": -b}[d[1]]
            start = d[2]
            h += d[3] * max(0.0, (x - start) / (1.0 - start)) ** d[4]
        elif kind == "corner":
            x = (d[1] * a + d[2] * b) * 0.5
            start = d[3]
            h += d[4] * max(0.0, (x - start) / (1.0 - start)) ** 1.6
        elif kind == "fold":
            n = math.hypot(d[1], d[2])
            x = (d[1] * s + d[2] * t) / n
            h += d[4] * max(0.0, x - d[3])
        elif kind == "wave":
            h += d[1] * (0.5 + 0.5 * math.sin(math.pi * (d[2] * a + d[3] * b) + d[4]))
    return h


def to_local(sheet, x, y):
    c, sn = math.cos(math.radians(sheet["rot"])), math.sin(math.radians(sheet["rot"]))
    dx, dy = x - sheet["c"][0], y - sheet["c"][1]
    return c * dx + sn * dy, -sn * dx + c * dy


def to_world(sheet, s, t):
    c, sn = math.cos(math.radians(sheet["rot"])), math.sin(math.radians(sheet["rot"]))
    return sheet["c"][0] + c * s - sn * t, sheet["c"][1] + sn * s + c * t


def grid_st(sheet, i, j):
    w, l = sheet["size"]
    return -w * 0.5 + w * i / NU, -l * 0.5 + l * j / NV


def top_height_at(sheet, x, y):
    """Top surface height of an already built sheet at world xy (clamped to the sheet) and distance outside."""
    w, l = sheet["size"]
    s, t = to_local(sheet, x, y)
    sc, tc = min(max(s, -w * 0.5), w * 0.5), min(max(t, -l * 0.5), l * 0.5)
    dout = math.hypot(s - sc, t - tc)
    fi, fj = (sc / w + 0.5) * NU, (tc / l + 0.5) * NV
    i0, j0 = min(int(fi), NU - 1), min(int(fj), NV - 1)
    u, v = fi - i0, fj - j0
    z = sheet["zb"]
    zb = (z[i0][j0] * (1 - u) * (1 - v) + z[i0 + 1][j0] * u * (1 - v)
          + z[i0][j0 + 1] * (1 - u) * v + z[i0 + 1][j0 + 1] * u * v)
    return zb + THICK, dout


def fit_layout(sheets, lo, hi):
    """Move sheet centers (sizes fixed) so the union footprint matches the target XY bounds."""
    for _ in range(60):
        xs, ys = [], []
        for sh in sheets:
            for sx in (-0.5, 0.5):
                for sy in (-0.5, 0.5):
                    x, y = to_world(sh, sx * sh["size"][0], sy * sh["size"][1])
                    xs.append(x)
                    ys.append(y)
        cur = ((min(xs), max(xs)), (min(ys), max(ys)))
        done = True
        for ax in range(2):
            c_lo, c_hi = cur[ax]
            t_lo, t_hi = lo[ax], hi[ax]
            if abs(c_lo - t_lo) > 1e-4 or abs(c_hi - t_hi) > 1e-4:
                done = False
            k = (t_hi - t_lo) / (c_hi - c_lo)
            cc, tc = (c_lo + c_hi) * 0.5, (t_lo + t_hi) * 0.5
            for sh in sheets:
                c = list(sh["c"])
                c[ax] = tc + (c[ax] - cc) * (1.0 + (k - 1.0) * 0.7)
                sh["c"] = tuple(c)
        if done:
            break


def page_uv(sheet, s, t):
    w, l = sheet["size"]
    fu, fv = s / w + 0.5, t / l + 0.5
    if sheet.get("flip"):
        fu, fv = 1.0 - fu, 1.0 - fv
    page = sheet["page"]
    if page is None:
        u0, v0, u1, v1 = BLANK_UV
        return u0 + fu * (u1 - u0), v0 + fv * (v1 - v0)
    pad = 1.0 / TEX_SIZE
    u = page * 0.5 + pad + fu * (0.5 - 2 * pad)
    v = PAGE_V0 + pad + fv * (1.0 - PAGE_V0 - 2 * pad)
    return u, v


def back_uv(sheet, s, t):
    w, l = sheet["size"]
    u0, v0, u1, v1 = BLANK_UV
    return u0 + (s / w + 0.5) * (u1 - u0), v0 + (t / l + 0.5) * (v1 - v0)


def build_heights(sheets, z_floor):
    built = []
    for sh in sheets:
        sh.setdefault("size", A4)
        zb = [[0.0] * (NV + 1) for _ in range(NU + 1)]
        for i in range(NU + 1):
            for j in range(NV + 1):
                s, t = grid_st(sh, i, j)
                z = z_floor + sheet_height(sh, s, t)
                x, y = to_world(sh, s, t)
                for low in built:
                    ztop, dout = top_height_at(low, x, y)
                    z = max(z, ztop + GAP - DRAPE_SLOPE * dout)
                zb[i][j] = z
        sh["zb"] = zb
        built.append(sh)


def scale_heights(sheets, z_floor, z_top_target):
    """Uniformly scale the height profile so the top of the pile matches the original bounds."""
    top = max(z for sh in sheets for row in sh["zb"] for z in row) + THICK
    f = (z_top_target - z_floor - THICK) / (top - z_floor - THICK)
    for sh in sheets:
        sh["zb"] = [[z_floor + (z - z_floor) * f for z in row] for row in sh["zb"]]
    return f


def add_sheet(bm, uv_layer, sh):
    """Closed slab: top grid (printed side), bottom grid, side walls around the border."""
    top, bot = {}, {}
    for i in range(NU + 1):
        for j in range(NV + 1):
            s, t = grid_st(sh, i, j)
            x, y = to_world(sh, s, t)
            z = sh["zb"][i][j]
            bot[i, j] = bm.verts.new((x, y, z))
            top[i, j] = bm.verts.new((x, y, z + THICK))

    def face(verts, uvs):
        f = bm.faces.new(verts)
        f.material_index = 0
        for loop, uv in zip(f.loops, uvs):
            loop[uv_layer].uv = uv
        return f

    for i in range(NU):
        for j in range(NV):
            idx = [(i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1)]
            st = [grid_st(sh, *k) for k in idx]
            face([top[k] for k in idx], [page_uv(sh, *p) for p in st])
            face([bot[k] for k in reversed(idx)], [back_uv(sh, *p) for p in reversed(st)])

    border = ([(i, 0) for i in range(NU)] + [(NU, j) for j in range(NV)]
              + [(i, NV) for i in range(NU, 0, -1)] + [(0, j) for j in range(NV, 0, -1)])
    u0, v0, u1, v1 = BLANK_UV
    for n, k0 in enumerate(border):
        k1 = border[(n + 1) % len(border)]
        uvs = [(u0, v0), (u0 + 0.01, v0), (u0 + 0.01, v0 + 0.002), (u0, v0 + 0.002)]
        face([bot[k0], bot[k1], top[k1], top[k0]], uvs)


def create_prop(name, spec, mat):
    lo, hi = ue_to_blender_bounds(spec["ue_min"], spec["ue_max"])
    sheets = [dict(s) for s in spec["sheets"]]
    for sh in sheets:
        sh.setdefault("size", A4)
    fit_layout(sheets, lo, hi)
    build_heights(sheets, lo[2])
    f = scale_heights(sheets, lo[2], hi[2])
    print(f"{name}: height scale factor {f:.2f}")
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.verify()
    for sh in sheets:
        add_sheet(bm, uv_layer, sh)
    obj = rh.finish_object(bm, name, [mat], keep_uvs=True, sharp_angle_deg=50.0)
    return obj


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Debris_Paper_Set1/5/6 (loose paper sheets).",
        Pivot="Floor level under the pile (same as the originals). Z up.")
    image = build_texture()
    mat = rh.make_material(MAT_PAPER, (1.0, 1.0, 1.0, 1.0), 0.0, 0.85, image=image)
    objs = [create_prop(name, spec, mat) for name, spec in PROPS.items()]
    for obj in objs:
        rh.report(obj)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    for obj in objs:
        rh.export_fbx(obj, EXPORT_DIR / f"{obj.name}.fbx")
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
