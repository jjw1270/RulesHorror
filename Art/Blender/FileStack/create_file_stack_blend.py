import math
import random
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready messy stack of document folders.
# Replaces /Game/Assets/BrutalistLevelKit/Models/Props/Items/SM_File_Pocket7 (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = bottom of the stack (z = 0), footprint centered like the original, Z up, meters.
#   Open edges with papers sticking out face Unreal +Y (Blender -Y) and Unreal +X; folder spines face Unreal -Y.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "FileStack.blend"
EXPORT_DIR = BASE_DIR / "Exports"
TEXTURE_DIR = BASE_DIR / "Textures"
MESH_NAME = "SM_FileStack"
OUTPUT_FBX = EXPORT_DIR / f"{MESH_NAME}.fbx"
TEXTURE_PAPER = TEXTURE_DIR / "T_FileStack_Paper.png"

# Target bounds in Blender meters (original Unreal cm: min (-29.0, -19.8, 0.0), max (28.4, 22.7, 16.8)).
TARGET_MIN = Vector((-0.290, -0.227, 0.000))
TARGET_MAX = Vector((0.284, 0.198, 0.168))

# Material slots (slot names in Unreal come from these names).
MAT_GREEN = "Folder_Board_Green"
MAT_GREY = "Folder_Board_Grey"
MAT_MANILA = "Folder_Board_Manila"
MAT_PAPER = "Paper_Stack"
I_GREEN, I_GREY, I_MANILA, I_PAPER = 0, 1, 2, 3
MATERIALS = [
    # name, base color (linear), metallic, roughness
    (MAT_GREEN, (0.040, 0.085, 0.062, 1.0), 0.0, 0.75),
    (MAT_GREY, (0.105, 0.130, 0.150, 1.0), 0.0, 0.70),
    (MAT_MANILA, (0.420, 0.340, 0.215, 1.0), 0.0, 0.80),
    (MAT_PAPER, (1.0, 1.0, 1.0, 1.0), 0.0, 0.85),  # textured with T_FileStack_Paper
]

# Texture atlas regions (UV space) of T_FileStack_Paper:
#   A: u 0.0-0.5, v 0.0-1.0  stacked sheet edges (horizontal lines), for paper block sides
#   B: u 0.5-1.0, v 0.5-1.0  printed page (grey dash "text", heading bar, form boxes), for the top sheet
#   C: u 0.5-1.0, v 0.0-0.5  plain aged paper, for sheet faces
TEX_SIZE = 1024
EDGE_V_METERS = 0.04  # region A: 1.0 of v = 4 cm of stacked paper

COVER_T = 0.0024
SHEET_T = 0.0007
GAP = 0.0010

rng = random.Random(7)


# -----------------------------------------------------------------------------
# Texture
# -----------------------------------------------------------------------------


def smooth_noise(h, w, cells, nrng):
    """Cheap value noise: random grid upsampled with bilinear interpolation."""
    g = nrng.random((cells + 1, cells + 1))
    ys = np.linspace(0, cells, h, endpoint=False)
    xs = np.linspace(0, cells, w, endpoint=False)
    y0, x0 = ys.astype(int), xs.astype(int)
    fy, fx = (ys - y0)[:, None], (xs - x0)[None, :]
    a = g[y0][:, x0]
    b = g[y0][:, x0 + 1]
    c = g[y0 + 1][:, x0]
    d = g[y0 + 1][:, x0 + 1]
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def paper_base(h, w, nrng):
    base = np.array([0.86, 0.845, 0.79])
    n = 0.6 * smooth_noise(h, w, 6, nrng) + 0.4 * smooth_noise(h, w, 24, nrng)
    fib = nrng.random((h, w)) * 0.035
    shade = 0.93 + 0.08 * n - fib
    img = base[None, None, :] * shade[:, :, None]
    # faint yellowing blotches
    yel = smooth_noise(h, w, 4, nrng)[:, :, None]
    img = img * (1.0 - 0.06 * yel * np.array([0.0, 0.3, 1.0])[None, None, :])
    return img


def draw_rect(img, x0, y0, x1, y1, color, alpha=1.0):
    h, w = img.shape[:2]
    x0, x1 = max(0, int(x0)), min(w, int(x1))
    y0, y1 = max(0, int(y0)), min(h, int(y1))
    if x1 > x0 and y1 > y0:
        img[y0:y1, x0:x1] = img[y0:y1, x0:x1] * (1 - alpha) + np.array(color) * alpha


def draw_frame(img, x0, y0, x1, y1, t, color, alpha=1.0):
    draw_rect(img, x0, y0, x1, y0 + t, color, alpha)
    draw_rect(img, x0, y1 - t, x1, y1, color, alpha)
    draw_rect(img, x0, y0, x0 + t, y1, color, alpha)
    draw_rect(img, x1 - t, y0, x1, y1, color, alpha)


def dash_line(img, x0, x1, y, hgt, nrng, color=(0.30, 0.30, 0.31), alpha=0.85):
    x = x0
    while x < x1:
        word = nrng.integers(10, 46)
        draw_rect(img, x, y, min(x + word, x1), y + hgt, color, alpha * nrng.uniform(0.75, 1.0))
        x += word + nrng.integers(5, 9)


def printed_page(n, nrng):
    """n x n printed form page; row 0 = bottom of the page (v = 0)."""
    img = paper_base(n, n, nrng) * np.array([1.0, 0.97, 0.90])[None, None, :]  # aged form paper
    ink = (0.28, 0.28, 0.30)
    m = int(n * 0.07)
    top = n - m
    # heading bar + sub heading
    draw_rect(img, m, top - 22, m + int(n * 0.42), top, ink, 0.9)
    dash_line(img, m, m + int(n * 0.30), top - 44, 9, nrng, ink)
    # stamp / reference box at the top right
    draw_frame(img, n - m - int(n * 0.28), top - 70, n - m, top, 4, ink, 0.8)
    for i in range(3):
        dash_line(img, n - m - int(n * 0.26), n - m - 12, top - 22 - i * 16, 7, nrng, ink, 0.6)
    # body text
    y = top - 100
    while y > n * 0.44:
        right = n - m - (nrng.integers(0, 160) if nrng.random() < 0.25 else 0)
        dash_line(img, m, right, y, 8, nrng, ink, 0.6)
        y -= 24
    # small table near the bottom
    t0, t1 = int(n * 0.12), int(n * 0.36)
    draw_frame(img, m, t0, n - m, t1, 3, ink, 0.8)
    for k in range(1, 4):
        draw_rect(img, m, t0 + k * (t1 - t0) // 4, n - m, t0 + k * (t1 - t0) // 4 + 2, ink, 0.6)
    for cx in (0.35, 0.62):
        xx = m + int((n - 2 * m) * cx)
        draw_rect(img, xx, t0, xx + 2, t1, ink, 0.6)
    for r in range(4):
        yy = t0 + r * (t1 - t0) // 4 + 12
        dash_line(img, m + 10, m + int((n - 2 * m) * 0.30), yy, 7, nrng, ink, 0.7)
        dash_line(img, m + int((n - 2 * m) * 0.38), m + int((n - 2 * m) * 0.55), yy, 7, nrng, ink, 0.7)
    # faint signature scribble block
    draw_rect(img, n - m - 140, int(n * 0.05), n - m - 20, int(n * 0.05) + 3, ink, 0.6)
    return img


def sheet_edges(h, w, nrng):
    """Stacked sheet edges: thin horizontal bands with dark gaps, slightly wavy."""
    rows = []
    while len(rows) < h * 2:
        t = int(nrng.integers(6, 15))
        tint = nrng.random()
        if tint < 0.18:
            col = np.array([0.78, 0.69, 0.52])  # manila sheet
        elif tint < 0.36:
            col = np.array([0.55, 0.62, 0.67]) * nrng.uniform(0.85, 1.0)  # blue-grey sheet
        else:
            col = np.array([0.86, 0.85, 0.81]) * nrng.uniform(0.75, 1.0)
        rows += [col] * t
        gap = int(nrng.integers(2, 5))
        rows += [col * nrng.uniform(0.22, 0.45)] * gap
    prof = np.array(rows[:h * 2])
    ux = np.arange(w)
    shift = (3.0 * np.sin(ux * 0.013 + 1.3) + 2.0 * np.sin(ux * 0.041)).astype(int)
    idx = (np.arange(h)[:, None] + shift[None, :]) % (h * 2)
    img = prof[idx]
    img *= (0.94 + 0.08 * smooth_noise(h, w, 16, nrng))[:, :, None]
    return img


def make_paper_texture():
    nrng = np.random.default_rng(11)
    n = TEX_SIZE
    half = n // 2
    img = np.zeros((n, n, 3))
    img[:, :half] = sheet_edges(n, half, nrng)
    img[half:, half:] = printed_page(half, nrng)
    img[:half, half:] = paper_base(half, half, nrng)
    rgba = np.concatenate([np.clip(img, 0, 1), np.ones((n, n, 1))], axis=2).astype(np.float32)
    image = bpy.data.images.new("T_FileStack_Paper", n, n, alpha=False)
    image.pixels.foreach_set(rgba.ravel())
    rh.save_image(image, TEXTURE_PAPER)
    # Reference the saved PNG from disk (a generated buffer would not survive saving the .blend).
    image.source = "FILE"
    image.filepath = str(TEXTURE_PAPER)
    image.reload()
    return image


# -----------------------------------------------------------------------------
# UV helpers (UVs are written in each part's local frame, then the part is transformed)
# -----------------------------------------------------------------------------


def bounds(bm):
    lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
    hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts)))
    return lo, hi


def region_uv(t, r0, r1, m=0.012):
    return r0 + m + max(0.0, min(1.0, t)) * (r1 - r0 - 2 * m)


def write_uvs(bm, mode, v_offset=0.0):
    """mode: 'box' (1 UV = 1 m), 'block' (sides -> sheet edges, caps -> plain paper),
    'sheet' (all faces plain paper), 'printed' (top face -> printed page)."""
    uv_layer = bm.loops.layers.uv.verify()
    lo, hi = bounds(bm)
    size = hi - lo
    size = Vector((max(size.x, 1e-6), max(size.y, 1e-6), max(size.z, 1e-6)))
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for loop in f.loops:
            co = loop.vert.co
            tx, ty = (co.x - lo.x) / size.x, (co.y - lo.y) / size.y
            if mode == "box":
                uv = (co.y, co.z) if ax == 0 else (co.x, co.z) if ax == 1 else (co.x, co.y)
            elif mode == "block" and ax != 2:
                th = ty if ax == 0 else tx
                uv = (region_uv(th, 0.0, 0.5), min(0.99, v_offset + (co.z - lo.z) / EDGE_V_METERS))
            elif mode == "printed" and ax == 2 and n.z > 0:
                uv = (region_uv(tx, 0.5, 1.0), region_uv(ty, 0.5, 1.0))
            else:
                uv = (region_uv(tx, 0.5, 1.0), region_uv(ty, 0.0, 0.5))
            loop[uv_layer].uv = uv


def merge_uv(dst, src):
    """Like rh.merge_into, but also copies the UV layer."""
    duv = dst.loops.layers.uv.verify()
    suv = src.loops.layers.uv.verify()
    vmap = {v: dst.verts.new(v.co) for v in src.verts}
    for f in src.faces:
        nf = dst.faces.new([vmap[v] for v in f.verts])
        nf.material_index = f.material_index
        for sl, dl in zip(f.loops, nf.loops):
            dl[duv].uv = sl[suv].uv
    src.free()


# -----------------------------------------------------------------------------
# Parts (local frame: footprint centered at origin, z from 0 up)
# -----------------------------------------------------------------------------


def cover_slab(w, d, t, inner_rect, droops, mat_index):
    """Folder cover as a 4x4 vertex slab. The ring outside inner_rect (x0, x1, y0, y1) drops by droops
    (per corner: -x-y, +x-y, +x+y, -x+y), so the free edges hang over the paper block."""
    xs = [-w / 2, max(-w / 2 + 0.002, inner_rect[0]), min(w / 2 - 0.002, inner_rect[1]), w / 2]
    ys = [-d / 2, max(-d / 2 + 0.002, inner_rect[2]), min(d / 2 - 0.002, inner_rect[3]), d / 2]
    bm = bmesh.new()

    def droop(i, j):
        if 0 < i < 3 and 0 < j < 3:
            return 0.0
        fx = (xs[i] + w / 2) / w
        fy = (ys[j] + d / 2) / d
        a, b, c, e = droops
        return (a * (1 - fx) * (1 - fy) + b * fx * (1 - fy) + c * fx * fy + e * (1 - fx) * fy)

    top = [[bm.verts.new((xs[i], ys[j], t - droop(i, j))) for j in range(4)] for i in range(4)]
    bot = [[bm.verts.new((xs[i], ys[j], -droop(i, j))) for j in range(4)] for i in range(4)]
    for i in range(3):
        for j in range(3):
            bm.faces.new((top[i][j], top[i + 1][j], top[i + 1][j + 1], top[i][j + 1]))
            bm.faces.new((bot[i][j], bot[i][j + 1], bot[i + 1][j + 1], bot[i + 1][j]))
    ring = [(i, 0) for i in range(3)] + [(3, j) for j in range(3)] + \
           [(i, 3) for i in range(3, 0, -1)] + [(0, j) for j in range(3, 0, -1)]
    for k in range(len(ring)):
        (i0, j0), (i1, j1) = ring[k], ring[(k + 1) % len(ring)]
        bm.faces.new((bot[i0][j0], bot[i1][j1], top[i1][j1], top[i0][j0]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def paper_block(w, d, h, bevel=0.0015):
    bm = rh.beveled_box(-w / 2, w / 2, -d / 2, d / 2, 0.0, h, bevel, I_PAPER, segments=1)
    write_uvs(bm, "block", v_offset=rng.uniform(0.0, 0.4))
    return bm


def sheet(w, d, mat_index, printed=False):
    bm = rh.box(-w / 2, w / 2, -d / 2, d / 2, 0.0, SHEET_T, mat_index)
    write_uvs(bm, "printed" if printed else ("sheet" if mat_index == I_PAPER else "box"))
    return bm


def place(bm, angle_deg, dx, dy, z):
    rh.transform(bm, Matrix.Translation((dx, dy, z)) @ Matrix.Rotation(math.radians(angle_deg), 4, "Z"))
    return bm


def protruding_sheets(dst, w, d, z0, h, count, mats, angle, dx, dy):
    """Loose sheets inside a folder/bundle (lower part of the paper block), poking out mostly at the
    front (Blender -Y = Unreal +Y) and right (+X) edges."""
    for _ in range(count):
        sw = w * rng.uniform(0.86, 0.97)
        sd = d * rng.uniform(0.86, 0.97)
        ox = rng.choice((-1.0, 1.0, 1.0)) * rng.uniform(0.010, 0.050)
        oy = -rng.uniform(0.012, 0.045)
        zz = z0 + h * rng.uniform(0.15, 0.40)
        part = sheet(sw, sd, rng.choice(mats))
        rh.transform(part, Matrix.Translation((ox, oy, 0.0)) @ Matrix.Rotation(math.radians(rng.uniform(-5, 5)), 4, "Z"))
        merge_uv(dst, place(part, angle, dx, dy, zz))


def folder(dst, z, h, board, angle, dx, dy, scale=1.0, sheets=2, sheet_mats=(I_PAPER,), printed_top=False,
           cover_t=COVER_T):
    w = 0.490 * scale + rng.uniform(-0.008, 0.008)
    d = 0.370 * scale + rng.uniform(-0.006, 0.006)
    angle += rng.uniform(-0.8, 0.8)
    inner = h - 2 * cover_t
    # Bottom cover (flat), paper block pushed against the spine, top cover whose free edges droop
    # over the papers, spine at the back (Blender +Y = Unreal -Y).
    bottom = rh.beveled_box(-w / 2, w / 2, -d / 2, d / 2, 0.0, cover_t, 0.0008, board, segments=1)
    write_uvs(bottom, "box")
    merge_uv(dst, place(bottom, angle, dx, dy, z))
    bw, bd = w - rng.uniform(0.012, 0.026), d - rng.uniform(0.018, 0.032)
    bx, by = rng.uniform(-0.003, 0.003), (d - bd) / 2 - 0.003
    block = paper_block(bw, bd, inner)
    rh.transform(block, Matrix.Translation((bx, by, 0.0)))
    merge_uv(dst, place(block, angle, dx, dy, z + cover_t))
    max_droop = min(0.0045, inner * 0.45)
    droops = [rng.uniform(0.5, 1.0) * max_droop, rng.uniform(0.3, 1.0) * max_droop, 0.0, rng.uniform(0.0, 0.3) * max_droop]
    inner_rect = (bx - bw / 2 - 0.001, bx + bw / 2 + 0.001, by - bd / 2 - 0.001, d / 2 - 0.006)
    top = cover_slab(w + rng.uniform(-0.004, 0.004), d, cover_t, inner_rect, droops, board)
    write_uvs(top, "box")
    merge_uv(dst, place(top, angle, dx, dy, z + h - cover_t))
    spine = rh.beveled_box(-w / 2, w / 2, d / 2 - 0.004, d / 2 + 0.0015, 0.0, h, min(0.002, h * 0.3), board, segments=1)
    write_uvs(spine, "box")
    merge_uv(dst, place(spine, angle, dx, dy, z))
    if sheets:
        protruding_sheets(dst, bw, bd, z + cover_t, inner, sheets, sheet_mats, angle, dx + bx, dy + by)
    if printed_top:
        pw, pd = w * 0.84, d * 0.78
        page = sheet(pw, pd, I_PAPER, printed=True)
        rh.transform(page, Matrix.Translation((0.010, -0.010, 0.0)) @ Matrix.Rotation(math.radians(-2.0), 4, "Z"))
        merge_uv(dst, place(page, angle, dx, dy, z + h + 0.0004))
    return z + h + GAP


def bundle(dst, z, h, angle, dx, dy, scale=1.0, sheets=2, sheet_mats=(I_PAPER,), top_mat=I_PAPER):
    w = 0.455 * scale + rng.uniform(-0.010, 0.010)
    d = 0.335 * scale + rng.uniform(-0.008, 0.008)
    angle += rng.uniform(-1.0, 1.0)
    block = paper_block(w, d, h - SHEET_T)
    merge_uv(dst, place(block, angle, dx, dy, z))
    protruding_sheets(dst, w, d, z, h, sheets, sheet_mats, angle, dx, dy)
    # top sheet, slightly turned and shifted
    page = sheet(w * rng.uniform(0.95, 1.03), d * rng.uniform(0.95, 1.03), top_mat)
    rh.transform(page, Matrix.Rotation(math.radians(rng.uniform(-3, 3)), 4, "Z"))
    merge_uv(dst, place(page, angle, dx + rng.uniform(-0.01, 0.02), dy - rng.uniform(0.0, 0.015), z + h - SHEET_T))
    return z + h + GAP


def create_mesh(mats):
    bm = bmesh.new()
    z = 0.0
    # bottom to top: (thickness, board, angle, dx, dy, ...) ; a little random jitter is added inside
    z = folder(bm, z, 0.026, I_GREEN, 1.5, 0.020, -0.010, scale=1.04, sheets=1, sheet_mats=(I_MANILA,),
               cover_t=0.0035)
    z = folder(bm, z, 0.014, I_GREEN, -2.5, 0.006, 0.004, sheets=2, sheet_mats=(I_MANILA, I_GREY))
    z = bundle(bm, z, 0.010, 3.5, 0.010, -0.006, sheets=2, sheet_mats=(I_PAPER, I_GREY), top_mat=I_GREY)
    z = folder(bm, z, 0.013, I_GREY, -1.0, -0.004, 0.006, scale=0.98, sheets=1, sheet_mats=(I_PAPER,))
    z = folder(bm, z, 0.016, I_GREEN, 2.5, 0.004, -0.008, sheets=2, sheet_mats=(I_PAPER, I_MANILA))
    z = bundle(bm, z, 0.008, -4.0, -0.006, -0.004, sheets=1, sheet_mats=(I_PAPER,), top_mat=I_MANILA)
    z = folder(bm, z, 0.013, I_GREEN, 0.5, -0.010, 0.002, sheets=2, sheet_mats=(I_GREY, I_PAPER))
    z = bundle(bm, z, 0.009, 4.0, -0.004, -0.010, sheets=2, sheet_mats=(I_PAPER, I_MANILA))
    z = folder(bm, z, 0.012, I_GREY, -2.0, -0.014, 0.004, scale=0.98, sheets=1, sheet_mats=(I_PAPER,))
    z = folder(bm, z, 0.014, I_GREEN, 1.5, -0.008, -0.004, sheets=2, sheet_mats=(I_PAPER, I_GREY))
    z = bundle(bm, z, 0.006, -3.0, -0.012, -0.006, sheets=1, sheet_mats=(I_MANILA,))
    z = folder(bm, z, 0.010, I_MANILA, -1.5, -0.022, -0.002, scale=1.02, sheets=1, sheet_mats=(I_PAPER,),
               printed_top=True)

    # Fit exactly into the original bounds (per-axis scale is within a few percent of 1).
    lo, hi = bounds(bm)
    for v in bm.verts:
        for i in range(3):
            t = (v.co[i] - lo[i]) / (hi[i] - lo[i])
            v.co[i] = TARGET_MIN[i] + t * (TARGET_MAX[i] - TARGET_MIN[i])
    print(f"Raw stack size before fit (m): {hi.x - lo.x:.3f} x {hi.y - lo.y:.3f} x {hi.z - lo.z:.3f}")
    return rh.finish_object(bm, MESH_NAME, mats, keep_uvs=True)


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_File_Pocket7 (messy stack of document folders).",
        Pivot="Bottom of the stack (z = 0), same footprint as the original. Open edges face Unreal +Y / +X.",
        ExportFBX=str(OUTPUT_FBX),
    )
    image = make_paper_texture()
    mats = []
    for name, color, metallic, roughness in MATERIALS:
        mats.append(rh.make_material(name, color, metallic, roughness, image=image if name == MAT_PAPER else None))
    obj = create_mesh(mats)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, OUTPUT_FBX)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    rh.report(obj)
    print(f"Saved blend: {OUTPUT_BLEND}")
    print(f"Exported FBX: {OUTPUT_FBX}")


if __name__ == "__main__":
    main()
