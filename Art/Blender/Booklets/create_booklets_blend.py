import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh

# RulesHorror booklet / newspaper generator.
#   SM_BookletOpen   : open stapled A5 report lying on its spine (replaces Deko_MatrixDemo SM_BooksPamphlets_G01_N1)
#   SM_NewspaperFlat : newspaper folded in half lying flat     (replaces Deko_MatrixDemo SM_BooksPamphlets_I01_N1)
# Same bounds and pivot as the originals (pivot = floor under the center), Z up, meters.
# Printing is suggested with grey dashes and dark bars only (no readable text, no real names).

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "Booklets.blend"
EXPORT_DIR = BASE_DIR / "Exports"
TEX_BOOKLET = BASE_DIR / "Textures" / "T_Booklet_Pages.png"
TEX_NEWSPAPER = BASE_DIR / "Textures" / "T_Newspaper_Folded.png"

BOOKLET = "SM_BookletOpen"
NEWSPAPER = "SM_NewspaperFlat"

TEX_SIZE = 1024

# --- Booklet (spine along X at y = 0; page top toward +X; left page on Blender +Y) ---
BK_X = 0.105            # cover half length along X (page height 21 cm)
BK_Y = 0.148            # cover half width along Y (A5 page width 14.8 cm)
BK_PAGE_INSET = 0.0015  # pages are slightly smaller than the cover
BK_GUTTER = 0.0012      # half gap between the two page blocks at the spine
BK_COVER_T = 0.0004
BK_PAGES_MIN = 0.0008   # page block thickness at the spine
BK_PAGES_MAX = 0.0035   # page block thickness away from the spine
BK_LIFT = 0.0115        # outer edges rise because the opened booklet springs back
BK_TOP_Z = 0.017        # original top
BK_NY, BK_NX = 12, 4
BK_SPREAD_V0 = 1.0 - 0.709  # spread printed in v [V0, 1]; blank card + page edge lines below

# --- Newspaper (fold edge on Blender -Y = Unreal +Y, open edges on Blender +Y, text reads along +X) ---
NP_X0, NP_X1 = -0.197, 0.182
NP_YF, NP_YO = -0.094, 0.093
NP_TOP_Z = 0.011
NP_NX, NP_NY = 12, 10
NP_FOLD_SEG = 6


# -----------------------------------------------------------------------------
# Texture helpers (images built top-left origin, flipped when written to Blender)
# -----------------------------------------------------------------------------


def smooth_noise(h, w, cells, rng):
    gh, gw = cells + 1, max(2, int(cells * w / h) + 1)
    g = rng.random((gh, gw))
    ys, xs = np.linspace(0, gh - 1, h), np.linspace(0, gw - 1, w)
    y0, x0 = np.minimum(ys.astype(int), gh - 2), np.minimum(xs.astype(int), gw - 2)
    fy, fx = (ys - y0)[:, None], (xs - x0)[None, :]
    return (g[y0][:, x0] * (1 - fx) * (1 - fy) + g[y0][:, x0 + 1] * fx * (1 - fy)
            + g[y0 + 1][:, x0] * (1 - fx) * fy + g[y0 + 1][:, x0 + 1] * fx * fy)


def ink(img, x0, y0, x1, y1, tone):
    h, w = img.shape[:2]
    x0, x1 = max(0, int(x0)), min(w, int(x1))
    y0, y1 = max(0, int(y0)), min(h, int(y1))
    if x1 > x0 and y1 > y0:
        img[y0:y1, x0:x1, :3] = np.minimum(img[y0:y1, x0:x1, :3], tone)


def fill(img, x0, y0, x1, y1, color):
    h, w = img.shape[:2]
    x0, x1 = max(0, int(x0)), min(w, int(x1))
    y0, y1 = max(0, int(y0)), min(h, int(y1))
    img[y0:y1, x0:x1, :3] = color


def text_line(img, rng, x0, x1, y, height, tone, ragged=False, word=(8, 38)):
    x = x0
    end = x1 - (rng.uniform(0.2, 0.6) * (x1 - x0) if ragged else 0)
    while x < end:
        wlen = rng.integers(*word)
        ink(img, x, y, min(x + wlen, end), y + height, tone)
        x += wlen + rng.integers(4, 8)


def paragraph(img, rng, x0, x1, y, lines, pitch=12, height=4, tone=0.45, word=(8, 38)):
    for i in range(lines):
        text_line(img, rng, x0, x1, y + i * pitch, height, tone, ragged=(i == lines - 1), word=word)
    return y + lines * pitch


def table(img, rng, x0, y0, cols, rows, cw, rh_px, tone=0.5):
    w, h = cols * cw, rows * rh_px
    ink(img, x0, y0, x0 + w, y0 + rh_px, 0.8)
    for r in range(rows + 1):
        ink(img, x0, y0 + r * rh_px, x0 + w, y0 + r * rh_px + 1, tone)
    for c in range(cols + 1):
        ink(img, x0 + c * cw, y0, x0 + c * cw + 1, y0 + h, tone)
    for r in range(rows):
        for c in range(cols):
            ln = rng.integers(int(cw * 0.25), int(cw * 0.7))
            ink(img, x0 + c * cw + 6, y0 + r * rh_px + rh_px // 2 - 2, x0 + c * cw + 6 + ln,
                y0 + r * rh_px + rh_px // 2 + 2, 0.32 if r == 0 else 0.5)
    return y0 + h


def bar_chart(img, rng, x0, y0, w, h):
    ink(img, x0, y0, x0 + w, y0 + 1, 0.5)
    ink(img, x0, y0 + h, x0 + w, y0 + h + 1, 0.5)
    ink(img, x0, y0, x0 + 1, y0 + h, 0.5)
    ink(img, x0 + w, y0, x0 + w + 1, y0 + h + 1, 0.5)
    n = 7
    bw = (w - 30) // n
    for i in range(n):
        bh = int(rng.uniform(0.25, 0.85) * (h - 20))
        bx = x0 + 18 + i * bw
        ink(img, bx, y0 + h - bh, bx + bw - 8, y0 + h, 0.42 if i % 2 else 0.6)
    for k in range(1, 4):
        ink(img, x0 + 10, y0 + k * h // 4, x0 + w - 10, y0 + k * h // 4 + 1, 0.82)


def age(img, rng, strength, tint=(1.0, 0.99, 0.965), cells=(6, 24)):
    h, w = img.shape[:2]
    n = 0.07 * smooth_noise(h, w, cells[0], rng) + 0.04 * smooth_noise(h, w, cells[1], rng)
    dirt = 1.0 - strength * n
    for c in range(3):
        img[:, :, c] *= dirt * tint[c]


def edge_lines(img, rng, base, period=3):
    """Stacked sheet edges: horizontal light/dark lines (constant along u)."""
    h, w = img.shape[:2]
    rows = np.arange(h)
    shade = np.where(rows % period == 0, 0.80, 1.0) * (0.96 + 0.04 * rng.random(h))
    img[:, :, :3] = np.asarray(base)[None, None, :] * shade[:, None, None]
    img[:, :, :3] *= (0.97 + 0.03 * smooth_noise(h, w, 12, rng))[:, :, None]


def to_blender_image(name, img, path):
    img = np.clip(img, 0.0, 1.0)
    h, w = img.shape[:2]
    image = bpy.data.images.new(name, w, h, alpha=False)
    image.pixels.foreach_set(np.flipud(img).astype(np.float32).ravel())
    rh.save_image(image, path)
    return image


# -----------------------------------------------------------------------------
# Booklet texture: spread on top, blank cover card (left) and page-edge lines (right) below
# -----------------------------------------------------------------------------


def booklet_texture():
    rng = np.random.default_rng(11)
    S = TEX_SIZE
    paper = np.array([0.95, 0.945, 0.92])
    img = np.ones((S, S, 4))
    img[:, :, :3] = paper
    img[:, :, :3] -= (smooth_noise(S, S, 160, rng) * 0.02 + rng.random((S, S)) * 0.01)[:, :, None]
    sh = int(round(S * (1.0 - BK_SPREAD_V0)))
    half = S // 2
    # Left page (inner margin on the right, next to the gutter).
    left = img[0:sh, 0:half]
    ml, mr = 40, half - 52
    ink(left, ml, 26, ml + 90, 30, 0.6)
    ink(left, ml, 36, mr, 37, 0.6)
    y = 58
    ink(left, ml, y, ml + 210, y + 14, 0.25)
    y += 30
    y = paragraph(left, rng, ml, mr, y, 7) + 10
    y = paragraph(left, rng, ml, mr, y, 5) + 18
    bar_chart(left, rng, ml + 10, y, mr - ml - 20, 150)
    y += 162
    text_line(left, rng, ml + 10, ml + 200, y, 3, 0.55)
    y += 20
    y = paragraph(left, rng, ml, mr, y, 8) + 10
    y = paragraph(left, rng, ml, mr, y, 4) + 10
    paragraph(left, rng, ml, mr, y, 5)
    ink(left, ml, sh - 34, ml + 14, sh - 28, 0.45)
    # Right page.
    right = img[0:sh, half:S]
    ml, mr = 52, half - 40
    ink(right, mr - 90, 26, mr, 30, 0.6)
    ink(right, ml, 36, mr, 37, 0.6)
    y = 58
    y = paragraph(right, rng, ml, mr, y, 6) + 14
    ink(right, ml, y, ml + 150, y + 9, 0.3)
    y += 22
    y = paragraph(right, rng, ml, mr, y, 5) + 16
    y = table(right, rng, ml, y, 4, 8, (mr - ml) // 4, 20) + 18
    y = paragraph(right, rng, ml, mr, y, 6) + 10
    y = paragraph(right, rng, ml, mr, y, 5) + 10
    paragraph(right, rng, ml, mr, y, 6)
    ink(right, mr - 14, sh - 34, mr, sh - 28, 0.45)
    # Gutter shadow and soft wear on the spread.
    xs = np.arange(S)
    gutter = 1.0 - 0.10 * np.exp(-((xs - half) / 18.0) ** 2)
    img[0:sh, :, :3] *= gutter[None, :, None]
    age(img[0:sh], rng, 0.6)
    # Lower strip: blank cover card (left half) and page edges (right half).
    age(img[sh:, 0:half], rng, 0.8)
    edge_lines(img[sh:, half:S], rng, paper * 0.97)
    return to_blender_image("T_Booklet_Pages", img, TEX_BOOKLET)


# Atlas regions (u0, v0, u1, v1)
BK_UV_COVER = (0.02, 0.02, 0.48, BK_SPREAD_V0 - 0.02)
BK_UV_EDGES = (0.52, 0.02, 0.98, BK_SPREAD_V0 - 0.02)


# -----------------------------------------------------------------------------
# Newspaper texture: front page top half (upper half of the atlas), back columns, sheet edges
# -----------------------------------------------------------------------------


def photo(img, rng, x0, y0, x1, y1):
    """Generic grainy street-scene photo: light sky, block silhouettes, dark foreground shapes."""
    h, w = int(y1 - y0), int(x1 - x0)
    yy = np.linspace(0, 1, h)[:, None] * np.ones((1, w))
    shade = 0.78 - 0.18 * yy + 0.08 * (smooth_noise(h, w, 6, rng) - 0.5)
    x = 0
    while x < w:
        bw = rng.integers(30, 90)
        top = int(h * rng.uniform(0.15, 0.55))
        tone = rng.uniform(0.28, 0.55)
        shade[top:, x:x + bw] = tone + 0.06 * (smooth_noise(h - top, min(bw, w - x), 3, rng) - 0.5)
        for wy in range(top + 10, int(h * 0.7), 18):           # window rows
            for wx in range(x + 6, min(x + bw - 8, w - 8), 14):
                shade[wy:wy + 7, wx:wx + 6] = tone + 0.2
        x += bw + rng.integers(2, 12)
    for _ in range(6):                                          # dark foreground figures / cars
        cx, cw = rng.integers(0, w - 30), rng.integers(12, 60)
        ch = rng.integers(20, int(h * 0.3))
        shade[h - ch:h, cx:cx + cw] = rng.uniform(0.08, 0.2)
    shade += 0.05 * (smooth_noise(h, w, 30, rng) - 0.5)
    # Halftone-like dot texture.
    dots = 0.94 + 0.06 * (((np.arange(h)[:, None] // 2) + (np.arange(w)[None, :] // 2)) % 2)
    region = img[int(y0):int(y1), int(x0):int(x1), :3]
    region[:] = (shade * dots)[:, :, None]
    ink(img, x0, y0, x1, y0 + 2, 0.15)
    ink(img, x0, y1 - 2, x1, y1, 0.15)
    ink(img, x0, y0, x0 + 2, y1, 0.15)
    ink(img, x1 - 2, y0, x1, y1, 0.15)


def newspaper_texture():
    rng = np.random.default_rng(23)
    S = TEX_SIZE
    paper = np.array([0.86, 0.85, 0.80])
    img = np.ones((S, S, 4))
    img[:, :, :3] = paper
    img[:, :, :3] -= (smooth_noise(S, S, 200, rng) * 0.03 + rng.random((S, S)) * 0.015)[:, :, None]

    # Front page, upper half (fold at the bottom of this region). 1024 x 512 px.
    fp = img[0:S // 2, :]
    ml, mr = 28, S - 28
    # Masthead: chunky letter-like blocks centred, flanked by two small boxes.
    x, y0, y1 = 300, 26, 74
    while x < 724:
        wl = rng.integers(16, 34)
        ink(fp, x, y0 + rng.integers(0, 6), x + wl, y1, 0.16)
        x += wl + rng.integers(5, 10)
    ink(fp, ml, 26, ml + 120, 74, 0.55)
    ink(fp, ml + 4, 30, ml + 116, 70, 0.86)
    paragraph(fp, rng, ml + 10, ml + 110, 36, 3, pitch=10, height=3, tone=0.4)
    fill(fp, mr - 52, 26, mr, 74, (0.25, 0.25, 0.25))
    ink(fp, mr - 40, 38, mr - 12, 62, 0.75)
    ink(fp, ml, 84, mr, 87, 0.15)
    text_line(fp, rng, ml, ml + 160, 92, 4, 0.4)
    text_line(fp, rng, 440, 590, 92, 4, 0.4)
    text_line(fp, rng, mr - 160, mr, 92, 4, 0.4)
    ink(fp, ml, 102, mr, 103, 0.15)
    ink(fp, ml, 105, mr, 106, 0.15)
    # Headline: one row of big dark bars across the page, then a sub-headline.
    x = ml + 8
    while x < mr - 60:
        wl = rng.integers(70, 190)
        ink(fp, x, 118, min(x + wl, mr - 8), 160, 0.1)
        x += wl + 24
    text_line(fp, rng, ml + 8, mr - 200, 172, 8, 0.3, word=(30, 80))
    # Two photos with captions, columns of text in between and below.
    photo(fp, rng, ml, 196, 440, 400)
    photo(fp, rng, 470, 196, 820, 400)
    text_line(fp, rng, ml, 400, 408, 3, 0.5)
    text_line(fp, rng, 470, 760, 408, 3, 0.5)
    col_x = [850, mr]
    ink(fp, 838, 196, 839, 500, 0.5)
    ink(fp, col_x[0], 196, col_x[1], 206, 0.2)
    paragraph(fp, rng, col_x[0], col_x[1], 216, 24, pitch=11, height=3, tone=0.42, word=(6, 30))
    for cx0, cx1 in ((ml, 220), (236, 440), (470, 640), (656, 820)):
        paragraph(fp, rng, cx0, cx1, 424, 7, pitch=11, height=3, tone=0.42, word=(6, 30))
    for xl in (228, 455, 648):
        ink(fp, xl, 424, xl + 1, 500, 0.55)
    age(fp, rng, 1.0, tint=(1.0, 0.985, 0.94))

    # Back side (lower-left quarter): plain text columns with a few headings.
    back = img[S // 2:S, 0:S // 2]
    for c in range(4):
        x0 = 16 + c * 124
        ink(back, x0, 18, x0 + 100, 30, 0.25)
        paragraph(back, rng, x0, x0 + 112, 40, 38, pitch=11, height=3, tone=0.45, word=(6, 28))
    age(back, rng, 1.0, tint=(1.0, 0.985, 0.94))
    # Sheet edges (lower-right quarter).
    edge_lines(img[S // 2:S, S // 2:S], rng, paper * 0.98, period=3)
    return to_blender_image("T_Newspaper_Folded", img, TEX_NEWSPAPER)


NP_UV_FRONT = (0.0, 0.5, 1.0, 1.0)
NP_UV_BACK = (0.01, 0.01, 0.49, 0.49)
NP_UV_EDGES = (0.51, 0.01, 0.99, 0.49)


# -----------------------------------------------------------------------------
# Geometry helpers
# -----------------------------------------------------------------------------


def lerp_uv(region, fu, fv):
    u0, v0, u1, v1 = region
    return u0 + fu * (u1 - u0), v0 + fv * (v1 - v0)


def add_face(bm, uv_layer, verts, uvs, mat_index=0):
    f = bm.faces.new(verts)
    f.material_index = mat_index
    for loop, uv in zip(f.loops, uvs):
        loop[uv_layer].uv = uv
    return f


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3.0 - 2.0 * t)


# -----------------------------------------------------------------------------
# Booklet
# -----------------------------------------------------------------------------


def bk_lift(y, x):
    d = abs(y) / BK_Y
    return BK_LIFT * d * d + 0.0012 * d * (0.5 + 0.5 * math.sin(2.2 * x / BK_X + 0.6))


def bk_page_top(y, x):
    """Page block thickness above the cover at (y, x)."""
    d = abs(y) / BK_Y
    t = BK_PAGES_MIN + (BK_PAGES_MAX - BK_PAGES_MIN) * smoothstep(0.0, 0.12, d)
    t += 0.0018 * math.exp(-((d - 0.16) / 0.12) ** 2)       # pages bulge next to the gutter
    t += 0.0012 * max(0.0, (d - 0.85) / 0.15) ** 2           # top page edge springs up a little
    return t


def slab(bm, uv_layer, ys, xs, z_bot, z_top, top_uv, bot_uv, side_uv, mat_index=0):
    """Closed slab over a (y, x) grid. z_bot/z_top/top_uv/bot_uv are f(y, x); side_uv(along, frac_z, thick)."""
    ny, nx = len(ys), len(xs)
    B = [[bm.verts.new((xs[i], ys[j], z_bot(ys[j], xs[i]))) for i in range(nx)] for j in range(ny)]
    T = [[bm.verts.new((xs[i], ys[j], z_top(ys[j], xs[i]))) for i in range(nx)] for j in range(ny)]
    for j in range(ny - 1):
        for i in range(nx - 1):
            idx = [(j, i), (j, i + 1), (j + 1, i + 1), (j + 1, i)]
            add_face(bm, uv_layer, [T[a][b] for a, b in idx], [top_uv(ys[a], xs[b]) for a, b in idx], mat_index)
            ridx = list(reversed(idx))
            add_face(bm, uv_layer, [B[a][b] for a, b in ridx], [bot_uv(ys[a], xs[b]) for a, b in ridx], mat_index)
    border = ([(0, i) for i in range(nx - 1)] + [(j, nx - 1) for j in range(ny - 1)]
              + [(ny - 1, i) for i in range(nx - 1, 0, -1)] + [(j, 0) for j in range(ny - 1, 0, -1)])
    along = 0.0
    for n, (a0, b0) in enumerate(border):
        a1, b1 = border[(n + 1) % len(border)]
        seg = math.hypot(xs[b1] - xs[b0], ys[a1] - ys[a0])
        t0 = T[a0][b0].co.z - B[a0][b0].co.z
        t1 = T[a1][b1].co.z - B[a1][b1].co.z
        uvs = [side_uv(along, 0.0, t0), side_uv(along + seg, 0.0, t1),
               side_uv(along + seg, 1.0, t1), side_uv(along, 1.0, t0)]
        add_face(bm, uv_layer, [B[a0][b0], B[a1][b1], T[a1][b1], T[a0][b0]], uvs, mat_index)
        along += seg


def create_booklet(mat_paper, mat_staple):
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.verify()

    def edge_uv(along, fz, thick):
        u0, v0, u1, v1 = BK_UV_EDGES
        return u0 + min(along / 1.1, 1.0) * (u1 - u0), v0 + fz * min(thick / 0.004, 1.0) * (v1 - v0)

    def cover_uv(y, x):
        return lerp_uv(BK_UV_COVER, (BK_Y - y) / (2 * BK_Y), (x + BK_X) / (2 * BK_X))

    def spread_uv(y, x):
        pad = 1.0 / TEX_SIZE
        return lerp_uv((pad, BK_SPREAD_V0 + pad, 1.0 - pad, 1.0 - pad),
                       (BK_Y - y) / (2 * BK_Y), (x + BK_X) / (2 * BK_X))

    # Cover: one continuous card under both halves.
    ys = [BK_Y * (k / BK_NY) ** 1.3 for k in range(BK_NY + 1)]
    ys_full = [-v for v in reversed(ys[1:])] + ys
    xs = [-BK_X + 2 * BK_X * i / BK_NX for i in range(BK_NX + 1)]
    slab(bm, uv_layer, ys_full, xs,
         lambda y, x: bk_lift(y, x), lambda y, x: bk_lift(y, x) + BK_COVER_T,
         cover_uv, cover_uv, edge_uv)

    # Page blocks.
    px = BK_X - BK_PAGE_INSET
    xs_p = [-px + 2 * px * i / BK_NX for i in range(BK_NX + 1)]
    for side in (1.0, -1.0):
        y_in, y_out = BK_GUTTER, BK_Y - BK_PAGE_INSET
        ys_p = [side * (y_in + (y_out - y_in) * (k / BK_NY) ** 1.3) for k in range(BK_NY + 1)]
        if side < 0:
            ys_p = list(reversed(ys_p))
        base = lambda y, x: bk_lift(y, x) + BK_COVER_T + 0.00005
        slab(bm, uv_layer, ys_p, xs_p, base, lambda y, x: base(y, x) + bk_page_top(y, x),
             spread_uv, cover_uv, edge_uv)

    # Two staples in the gutter.
    z_gutter = BK_COVER_T + BK_PAGES_MIN
    for sx in (-0.055, 0.055):
        st = rh.beveled_box(sx - 0.006, sx + 0.006, -0.0004, 0.0004, z_gutter - 0.0002, z_gutter + 0.0004,
                            0.00015, mat_index=1, segments=1)
        rh.merge_into(bm, st)

    # Uniform height fit so the top matches the original bounds.
    top = max(v.co.z for v in bm.verts)
    f = BK_TOP_Z / top
    for v in bm.verts:
        v.co.z *= f
    print(f"{BOOKLET}: height scale factor {f:.2f}")
    return rh.finish_object(bm, BOOKLET, [mat_paper, mat_staple], keep_uvs=True, sharp_angle_deg=40.0)


# -----------------------------------------------------------------------------
# Folded newspaper
# -----------------------------------------------------------------------------


def np_profile(x):
    """Return bottom/top (y, z) stations from the fold to the open edge, at length position x."""
    th_fold = 0.0060
    r = th_fold * 0.5
    y_start = NP_YF + r
    pts = []
    wave = 0.0007 * math.sin(2 * math.pi * x / 0.22 + 0.8) + 0.0005 * math.sin(2 * math.pi * x / 0.09)
    for k in range(NP_NY + 1):
        d = k / NP_NY
        y = y_start + (NP_YO - y_start) * d
        zb = 0.0015 * smoothstep(0.55, 1.0, d)
        th = th_fold - 0.0016 * d
        bow = 0.0016 * math.sin(math.pi * d) + wave * math.sin(math.pi * d)
        lift = 0.0030 * max(0.0, (d - 0.7) / 0.3) ** 2
        pts.append((y, zb, zb + th + bow + lift))
    return pts, r


def create_newspaper(mat):
    bm = bmesh.new()
    uv_layer = bm.loops.layers.uv.verify()
    xs = [NP_X0 + (NP_X1 - NP_X0) * i / NP_NX for i in range(NP_NX + 1)]
    length = NP_X1 - NP_X0
    rings = []   # per x: list of verts around the closed profile
    ring_info = []
    for x in xs:
        pts, r = np_profile(x)
        loop = []
        info = []
        # bottom stations fold -> open
        for k, (y, zb, zt) in enumerate(pts):
            loop.append((y, zb))
            info.append(("bot", k))
        # top stations open -> fold
        for k in range(len(pts) - 1, -1, -1):
            y, zb, zt = pts[k]
            loop.append((y, zt))
            info.append(("top", k))
        # fold half-circle from top to bottom (center at y0, z = r)
        y0 = pts[0][0]
        zt0 = pts[0][2]
        rr = zt0 * 0.5
        for s in range(1, NP_FOLD_SEG):
            a = math.pi * 0.5 + math.pi * s / NP_FOLD_SEG
            loop.append((y0 + rr * math.cos(a) * 1.15, rr + rr * math.sin(a)))
            info.append(("fold", s))
        rings.append([bm.verts.new((x, y, z)) for (y, z) in loop])
        ring_info.append(info)

    n = len(rings[0])
    span_y = NP_YO - NP_YF

    def surf_uv(kind, idx, x, vert):
        fu = (x - NP_X0) / length
        if kind == "top":
            return lerp_uv(NP_UV_FRONT, fu, (vert.co.y - NP_YF) / span_y)
        if kind == "bot":
            return lerp_uv(NP_UV_BACK, fu, (vert.co.y - NP_YF) / span_y)
        return lerp_uv(NP_UV_BACK, fu, 0.0)

    for i in range(NP_NX):
        ra, rb = rings[i], rings[i + 1]
        for k in range(n):
            k2 = (k + 1) % n
            kind_a, kind_b = ring_info[i][k][0], ring_info[i][k2][0]
            quad = [ra[k], rb[k], rb[k2], ra[k2]]
            xsq = [xs[i], xs[i + 1], xs[i + 1], xs[i]]
            if kind_a == "bot" and kind_b == "top":
                # Open edge (sheet edges visible): lines along the length.
                uvs = [lerp_uv(NP_UV_EDGES, (xq - NP_X0) / length, (v.co.z) / 0.012) for v, xq in zip(quad, xsq)]
            elif kind_a == "top" and kind_b == "top":
                uvs = [surf_uv("top", 0, xq, v) for v, xq in zip(quad, xsq)]
            elif kind_a == "bot" and kind_b == "bot":
                uvs = [surf_uv("bot", 0, xq, v) for v, xq in zip(quad, xsq)]
            else:
                uvs = [surf_uv("fold", 0, xq, v) for v, xq in zip(quad, xsq)]
            add_face(bm, uv_layer, quad, uvs)

    # End caps (cut edges of all sheets): quads between bottom/top stations + fan over the fold.
    for ring, x, flip in ((rings[0], xs[0], True), (rings[-1], xs[-1], False)):
        m = NP_NY + 1
        bot = ring[0:m]
        top = list(reversed(ring[m:2 * m]))
        fold = ring[2 * m:]

        def cap_uv(v):
            return lerp_uv(NP_UV_EDGES, (v.co.y - NP_YF) / span_y, v.co.z / 0.012)

        faces = []
        for k in range(m - 1):
            faces.append([bot[k], bot[k + 1], top[k + 1], top[k]])
        fan = [top[0]] + fold + [bot[0]]
        for k in range(1, len(fan) - 1):
            faces.append([fan[0], fan[k], fan[k + 1]])
        for fv in faces:
            if flip:
                fv = list(reversed(fv))
            add_face(bm, uv_layer, fv, [cap_uv(v) for v in fv])

    top = max(v.co.z for v in bm.verts)
    f = NP_TOP_Z / top
    for v in bm.verts:
        v.co.z *= f
    print(f"{NEWSPAPER}: height scale factor {f:.2f}")
    return rh.finish_object(bm, NEWSPAPER, [mat], keep_uvs=True, sharp_angle_deg=40.0)


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacements for Deko_MatrixDemo SM_BooksPamphlets_G01_N1 / I01_N1.",
        Pivot="Floor under the center (same as the originals). Z up.")
    mat_pages = rh.make_material("Paper_Pages", (1.0, 1.0, 1.0, 1.0), 0.0, 0.8, image=booklet_texture())
    mat_staple = rh.make_material("Staple_Metal", (0.6, 0.6, 0.62, 1.0), 1.0, 0.3)
    mat_news = rh.make_material("Newspaper_Print", (1.0, 1.0, 1.0, 1.0), 0.0, 0.9, image=newspaper_texture())
    objs = [create_booklet(mat_pages, mat_staple), create_newspaper(mat_news)]
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
