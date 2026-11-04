import math
import sys
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Matrix

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready messy stack of CD jewel cases with a loose disc on top.
# Replaces /Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_Record_Stack_C01_N1 (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   Unreal bounds X -8.2..8.8, Y -8.6..7.4, Z 0.1..6.0 cm (pivot under the stack, Z up).
# Layout below is written in Unreal coordinates (cm, degrees from +X toward +Y) and converted
# to Blender (Y flipped) when the parts are placed.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "DiscStack.blend"
EXPORT_DIR = BASE_DIR / "Exports"
TEXTURE_PATH = BASE_DIR / "Textures" / "T_DiscStack_Print.png"
MESH_NAME = "SM_DiscStack"

# Material slots (slot names in Unreal come from these names). Linear colors.
MAT_CLEAR = "Case_Clear"      # jewel case shell, translucent
MAT_TRAY = "Tray_Black"       # disc tray + hinge
MAT_PRINT = "Print_Atlas"     # inserts + disc label (texture atlas)
MAT_DISC = "Disc_Silver"      # disc data side, hub and rim
I_CLEAR, I_TRAY, I_PRINT, I_DISC = 0, 1, 2, 3
CLEAR_COLOR = (0.55, 0.58, 0.62, 1.0)
CLEAR_OPACITY = 0.15

# Jewel case (meters). Local frame: x = 14.2 cm (hinge on -x), y = 12.5 cm, z up from the case bottom.
CASE_L, CASE_W, CASE_H = 0.142, 0.125, 0.0104
HINGE_W = 0.0055
DISC_R, DISC_HOLE, DISC_T = 0.060, 0.0075, 0.0012
TEX_SIZE = 1024

# Atlas tiles (u0, v0), each 0.5 x 0.5 in UV.
TILE_LABEL, TILE_BLUE, TILE_TEAL, TILE_GREY = (0.0, 0.0), (0.5, 0.0), (0.0, 0.5), (0.5, 0.5)

# Stack, bottom to top: (center x, center y [Unreal cm], yaw deg [Unreal], tilt about local y deg,
#                        tilt about local x deg, insert tile or None). Heights are stacked automatically.
STACK_BOTTOM = 0.10  # cm, like the original
CASES = [
    (-0.60, -0.20, 2.0, 0.0, 0.0, TILE_GREY),
    (-0.20, -0.90, -5.0, 0.30, -0.25, TILE_TEAL),
    (-0.30, 0.30, 8.0, -0.40, 0.20, TILE_GREY),
    (0.50, -0.10, 12.0, 0.45, 0.35, TILE_BLUE),
    (-0.45, -2.05, -2.0, -0.30, -0.30, None),
]
# Loose disc on top of the last case: offset from that case center (Unreal cm), tilt (deg about X, about Y).
DISC_OFFSET = (0.75, 0.25)
DISC_TILT = (0.4, -0.5)


# -----------------------------------------------------------------------------
# Texture
# -----------------------------------------------------------------------------


def _smooth_noise(rng, n, cells):
    """Cheap value noise: bilinear upsample of a random grid."""
    g = rng.random((cells + 1, cells + 1))
    t = np.linspace(0, cells, n, endpoint=False)
    i = t.astype(int)
    f = t - i
    f = f * f * (3 - 2 * f)
    a = g[i][:, i] * (1 - f)[None, :] + g[i][:, i + 1] * f[None, :]
    b = g[i + 1][:, i] * (1 - f)[None, :] + g[i + 1][:, i + 1] * f[None, :]
    return a * (1 - f)[:, None] + b * f[:, None]


def _print_rows(img, rng, x0, x1, y0, rows, row_h, gap, color, density=0.8):
    """Rows of grey dashes that read as small print (no real text)."""
    n = img.shape[0]
    for r in range(rows):
        y = y0 + r * (row_h + gap)
        x = x0
        while x < x1:
            w = int(rng.integers(row_h, row_h * 5))
            if rng.random() < density:
                img[int(y):int(y + row_h), int(x):int(min(x + w, x1))] = color
            x += w + row_h * 0.8
    return img


def make_texture():
    n = TEX_SIZE // 2
    rng = np.random.default_rng(7)
    yy, xx = np.mgrid[0:n, 0:n] / n
    atlas = np.zeros((TEX_SIZE, TEX_SIZE, 3))

    # CD label: off-white print with a colored band, a teal block and print rows.
    lab = np.ones((n, n, 3)) * np.array([0.86, 0.88, 0.90])
    r = np.hypot(xx - 0.5, yy - 0.5) * 2.0          # 1.0 = disc edge
    lab *= (0.94 + 0.06 * _smooth_noise(rng, n, 6))[..., None]
    band = (r > 0.84) & (r < 0.875)
    lab[band] = [0.10, 0.32, 0.55]
    lab[(r > 0.885) & (r < 0.895)] = [0.55, 0.12, 0.10]
    blk = (xx > 0.30) & (xx < 0.44) & (yy > 0.70) & (yy < 0.80)
    lab[blk] = [0.22, 0.48, 0.50]
    lab[(xx > 0.315) & (xx < 0.335) & (yy > 0.72) & (yy < 0.78)] = [0.85, 0.88, 0.88]
    lab = _print_rows(lab, rng, n * 0.62, n * 0.80, n * 0.30, 6, 6, 7, [0.45, 0.47, 0.50])
    lab = _print_rows(lab, rng, n * 0.20, n * 0.40, n * 0.30, 3, 5, 7, [0.55, 0.57, 0.60])
    lab[r > 0.985] = [0.70, 0.72, 0.75]
    atlas[0:n, 0:n] = lab

    # Blue / purple insert: diagonal gradient, waves and glow.
    d = (xx * 0.7 + yy * 0.3)
    wave = 0.5 + 0.5 * np.sin(xx * 9.0 + np.sin(yy * 5.0) * 2.0 + yy * 4.0)
    c1, c2, c3 = np.array([0.15, 0.30, 0.85]), np.array([0.50, 0.25, 0.85]), np.array([0.95, 0.40, 0.80])
    blue = c1 * (1 - d)[..., None] + c2 * d[..., None]
    blue = blue * (1 - 0.35 * wave[..., None]) + c3 * (0.35 * wave * d)[..., None]
    glow = np.exp(-(((xx - 0.72) / 0.18) ** 2 + ((yy - 0.30) / 0.12) ** 2))
    blue += np.array([0.25, 0.55, 0.95]) * glow[..., None] * 0.8
    blue *= (0.9 + 0.1 * _smooth_noise(rng, n, 10))[..., None]
    blue[(xx > 0.06) & (xx < 0.30) & (yy > 0.86) & (yy < 0.90)] = [0.85, 0.88, 0.95]
    blue = _print_rows(blue, rng, n * 0.06, n * 0.40, n * 0.06, 3, 5, 6, [0.70, 0.75, 0.90])
    atlas[0:n, n:] = np.clip(blue, 0, 1)

    # Teal insert: soft horizontal stripes and a dark block.
    stripes = 0.5 + 0.5 * np.sin(yy * 22.0 + _smooth_noise(rng, n, 4) * 3.0)
    teal = np.array([0.10, 0.45, 0.48]) * (1 - stripes[..., None]) + np.array([0.35, 0.72, 0.60]) * stripes[..., None]
    teal[(xx > 0.55) & (xx < 0.92) & (yy > 0.10) & (yy < 0.42)] *= 0.35
    teal = _print_rows(teal, rng, n * 0.58, n * 0.90, n * 0.15, 4, 5, 8, [0.75, 0.80, 0.78])
    atlas[n:, 0:n] = teal

    # Grey / white insert: printed booklet look.
    grey = np.ones((n, n, 3)) * np.array([0.78, 0.79, 0.80])
    grey *= (0.92 + 0.08 * _smooth_noise(rng, n, 8))[..., None]
    grey[(yy > 0.62) & (yy < 0.90) & (xx > 0.08) & (xx < 0.92)] = [0.30, 0.34, 0.40]
    circ = np.hypot(xx - 0.70, yy - 0.76) < 0.10
    grey[circ] = [0.75, 0.55, 0.20]
    grey = _print_rows(grey, rng, n * 0.08, n * 0.90, n * 0.10, 8, 6, 9, [0.40, 0.42, 0.45])
    atlas[n:, n:] = grey

    img = bpy.data.images.new("T_DiscStack_Print", TEX_SIZE, TEX_SIZE, alpha=False)
    rgba = np.ones((TEX_SIZE, TEX_SIZE, 4), dtype=np.float32)
    rgba[..., :3] = np.clip(atlas, 0, 1) ** 2.2   # authored in display space -> linear pixels
    img.pixels.foreach_set(rgba.ravel())
    rh.save_image(img, TEXTURE_PATH)
    img.filepath_raw = "//Textures/" + TEXTURE_PATH.name   # relative to the .blend (not packed)
    return img


# -----------------------------------------------------------------------------
# Geometry
# -----------------------------------------------------------------------------


def merge_uv(dst, src):
    """Like rh.merge_into but keeps the UV layer of src."""
    uv_d = dst.loops.layers.uv.verify()
    uv_s = src.loops.layers.uv.verify()
    vmap = {v: dst.verts.new(v.co) for v in src.verts}
    for f in src.faces:
        nf = dst.faces.new([vmap[v] for v in f.verts])
        nf.material_index = f.material_index
        for ld, ls in zip(nf.loops, f.loops):
            ld[uv_d].uv = ls[uv_s].uv
    src.free()
    return dst


def tile_uv(bm, faces, x0, x1, y0, y1, tile, margin=0.01):
    uv = bm.loops.layers.uv.verify()
    for f in faces:
        for loop in f.loops:
            s = (loop.vert.co.x - x0) / (x1 - x0)
            t = (loop.vert.co.y - y0) / (y1 - y0)
            loop[uv].uv = (tile[0] + margin + s * (0.5 - 2 * margin), tile[1] + margin + t * (0.5 - 2 * margin))


def jewel_case(insert_tile, ridges):
    bm = bmesh.new()
    hx0, hx1 = -CASE_L / 2, CASE_L / 2
    hy = CASE_W / 2
    # Translucent shell (lid + base), whole case outline except the hinge strip.
    rh.merge_into(bm, rh.beveled_box(hx0 + HINGE_W, hx1, -hy, hy, 0.0, CASE_H, 0.0009, I_CLEAR, segments=1))
    # Black disc tray inside the shell.
    rh.merge_into(bm, rh.box(hx0 + HINGE_W + 0.001, hx1 - 0.0015, -hy + 0.0012, hy - 0.0012, 0.0010, 0.0068, I_TRAY))
    # Hinge strip.
    rh.merge_into(bm, rh.beveled_box(hx0, hx0 + HINGE_W + 0.0004, -hy, hy, 0.0003, CASE_H - 0.0003, 0.0012,
                                     I_TRAY, segments=1))
    if ridges:
        n = 15
        for i in range(n):
            y = -hy + 0.009 + i * (CASE_W - 0.018) / (n - 1)
            rh.merge_into(bm, rh.box(hx0 + 0.0008, hx0 + HINGE_W - 0.0006, y - 0.0007, y + 0.0007,
                                     CASE_H - 0.0004, CASE_H + 0.0004, I_TRAY))
    bm.loops.layers.uv.verify()
    if insert_tile is not None:
        # Printed insert lying on the tray under the clear lid (planar UVs into its atlas tile).
        ins = rh.box(hx0 + HINGE_W + 0.0015, hx1 - 0.002, -hy + 0.0018, hy - 0.0018, 0.0068, 0.0072, I_PRINT)
        tile_uv(ins, ins.faces, hx0 + HINGE_W + 0.0015, hx1 - 0.002, -hy + 0.0018, hy - 0.0018, insert_tile)
        merge_uv(bm, ins)
    return bm


def disc():
    """CD lying flat, bottom at z = 0, centered at the origin. Label ring uses the atlas, the rest is silver."""
    seg = 40
    bm = bmesh.new()
    uv = bm.loops.layers.uv.verify()
    top_r = [DISC_HOLE, 0.0115, 0.0225, 0.0580, DISC_R]
    top_m = [I_DISC, I_DISC, I_PRINT, I_DISC]

    def ring(r, z):
        return [bm.verts.new((r * math.cos(2 * math.pi * k / seg), r * math.sin(2 * math.pi * k / seg), z))
                for k in range(seg)]

    tops = [ring(r, DISC_T) for r in top_r]
    bot_in, bot_out = ring(DISC_HOLE, 0.0), ring(DISC_R, 0.0)

    def band(a, b, mat):
        for k in range(seg):
            k2 = (k + 1) % seg
            f = bm.faces.new((a[k], a[k2], b[k2], b[k]))
            f.material_index = mat

    for i, mat in enumerate(top_m):
        band(tops[i], tops[i + 1], mat)       # top annuli
    band(tops[-1], bot_out, I_DISC)            # outer edge
    band(bot_out, bot_in, I_DISC)              # data side
    band(bot_in, tops[0], I_DISC)              # hole wall
    u0, v0 = TILE_LABEL
    for f in bm.faces:
        for loop in f.loops:
            co = loop.vert.co
            loop[uv].uv = (u0 + 0.25 + co.x / (2 * DISC_R) * 0.5, v0 + 0.25 + co.y / (2 * DISC_R) * 0.5)
    return bm


def place(cx_cm, cy_cm, yaw_deg, z_cm, tilt_y_deg=0.0, tilt_x_deg=0.0):
    """Matrix from part-local space to Blender space, given an Unreal-space placement."""
    return (Matrix.Translation((cx_cm / 100.0, -cy_cm / 100.0, z_cm / 100.0))
            @ Matrix.Rotation(math.radians(-yaw_deg), 4, "Z")
            @ Matrix.Rotation(math.radians(tilt_x_deg), 4, "X")
            @ Matrix.Rotation(math.radians(tilt_y_deg), 4, "Y"))


def create_mesh(mats):
    bm = bmesh.new()
    bm.loops.layers.uv.verify()
    top = STACK_BOTTOM
    z = top
    for i, (cx, cy, yaw, ty, tx, tile) in enumerate(CASES):
        # A tilted case dips below its center plane on one side and rises on the other.
        dip = (CASE_L * 50.0) * math.sin(math.radians(abs(ty))) + (CASE_W * 50.0) * math.sin(math.radians(abs(tx)))
        z = top + dip
        top = z + CASE_H * 100.0 + dip
        part = jewel_case(tile, ridges=(i == len(CASES) - 1))
        rh.transform(part, place(cx, cy, yaw, z, ty, tx))
        merge_uv(bm, part)
    cx, cy, yaw, ty, tx, _ = CASES[-1]
    a = math.radians(yaw)
    ox, oy = DISC_OFFSET
    dx, dy = ox * math.cos(a) - oy * math.sin(a), ox * math.sin(a) + oy * math.cos(a)
    part = disc()
    dip = DISC_R * 100.0 * (math.sin(math.radians(abs(DISC_TILT[0]))) + math.sin(math.radians(abs(DISC_TILT[1]))))
    rh.transform(part, place(cx + dx, cy + dy, yaw + 35.0, z + CASE_H * 100.0 + dip, DISC_TILT[1], DISC_TILT[0]))
    merge_uv(bm, part)

    # Box-project every face that is not printed (printed faces keep their atlas UVs).
    uv = bm.loops.layers.uv.verify()
    bm.normal_update()
    for f in bm.faces:
        if f.material_index == I_PRINT:
            continue
        ax = max(range(3), key=lambda i: abs(f.normal[i]))
        for loop in f.loops:
            co = loop.vert.co
            loop[uv].uv = (co.y, co.z) if ax == 0 else (co.x, co.z) if ax == 1 else (co.x, co.y)
    return rh.finish_object(bm, MESH_NAME, mats, sharp_angle_deg=40.0, keep_uvs=True)


def make_materials(image):
    clear = rh.make_material(MAT_CLEAR, CLEAR_COLOR, 0.0, 0.05)
    bsdf = next(n for n in clear.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Alpha"].default_value = CLEAR_OPACITY
    clear.diffuse_color = (CLEAR_COLOR[0], CLEAR_COLOR[1], CLEAR_COLOR[2], CLEAR_OPACITY)
    if hasattr(clear, "surface_render_method"):
        clear.surface_render_method = "BLENDED"
    tray = rh.make_material(MAT_TRAY, (0.012, 0.012, 0.014, 1.0), 0.0, 0.35)
    printed = rh.make_material(MAT_PRINT, (1.0, 1.0, 1.0, 1.0), 0.0, 0.45, image=image)
    silver = rh.make_material(MAT_DISC, (0.85, 0.86, 0.88, 1.0), 1.0, 0.12)
    return [clear, tray, printed, silver]


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for Deko_MatrixDemo SM_Record_Stack_C01_N1 (stack of CD cases).",
        Pivot="Same as the original: stack centered near the origin, Z up, bottom case at Z=0.1 cm.",
    )
    mats = make_materials(make_texture())
    obj = create_mesh(mats)
    rh.report(obj)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, EXPORT_DIR / f"{MESH_NAME}.fbx")
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
