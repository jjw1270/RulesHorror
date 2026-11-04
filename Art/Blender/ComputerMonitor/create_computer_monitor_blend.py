"""RulesHorror UE-ready interactive CRT monitor generator (SM_ComputerMonitor).

Replaces the purchased /Game/Assets/CRT_VCR_Material/Meshes/Monitor_r1, used by
/Game/Objects/Interactable/Computer_BP (component "Monitor"). This is the gameplay computer: the in-game
desktop UI is rendered into a render target and shown on the screen faces through a CRT screen material.

Contract with Unreal (all coordinates Unreal local cm; Blender meters, Blender +Y = Unreal -Y, X and Z same):
  * Bounds like the original: (-22.70, -22.41, -0.09) .. (22.70, 22.44, 44.34). Pivot = bottom center of the
    stand. The screen faces Unreal +Y (= Blender -Y).
  * Material slots: 0 = Plastic_Beige, 1 = Screen (C++ does SetMaterial(1, screenMID)), 2 = Plastic_Dark.
    Faces are also stored sorted by material index so slot order = first-use order in the FBX.
  * Screen = the UI widget quad: x -18.095..18.095, z 11.015..41.665. The glass is convex, Unreal y 19.0 at the
    edges/corners and 20.8 in the center, always in front of the hidden widget plane at Unreal y 18.24.
  * Screen UVs (Unreal): u = (x + 18.095) / 36.19, v = (41.665 - z) / 30.65 (u = 0 viewer's left, v = 0 top).
    Unreal flips V on FBX import, so Blender stores u = (x + 0.18095) / 0.3619, v = (z - 0.11015) / 0.3065.

Run:
  blender --background --factory-startup --python create_computer_monitor_blend.py
  (optional) ... --python create_computer_monitor_blend.py -- --preview <dir>
  The preview step renders the thumbnail angle, a rear view, a front view with a UV test grid on the screen
  (red marker must be top-left) and re-imports the FBX to check slot order and screen UVs. It runs after the
  .blend is saved and never saves it again.
"""

import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "ComputerMonitor.blend"
EXPORT_DIR = BASE_DIR / "Exports"
MESH_NAME = "SM_ComputerMonitor"
OUTPUT_FBX = EXPORT_DIR / f"{MESH_NAME}.fbx"

# Material slots in slot order: name, linear RGBA, metallic, roughness. Slot 1 MUST stay "Screen".
MAT_DEFS = [
    ("Plastic_Beige", (0.62, 0.58, 0.48, 1.0), 0.0, 0.55),
    ("Screen", (0.02, 0.025, 0.024, 1.0), 0.0, 0.15),
    ("Plastic_Dark", (0.08, 0.075, 0.07, 1.0), 0.0, 0.5),
]
I_BODY, I_SCREEN, I_DARK = 0, 1, 2

# Screen rectangle = in-game UI widget quad (Blender meters).
SCR_X0, SCR_X1 = -0.18095, 0.18095
SCR_Z0, SCR_Z1 = 0.11015, 0.41665
SCR_W, SCR_H = SCR_X1 - SCR_X0, SCR_Z1 - SCR_Z0
GLASS_Y_EDGE = -0.190   # Unreal y 19.0 at the edges and corners
GLASS_BULGE = 0.018     # Unreal y 20.8 in the center
GLASS_Y_BACK = -0.1875  # back of the glass slab (hidden inside the tunnel)
GLASS_NX, GLASS_NZ = 24, 20
WIDGET_Y = -0.1824      # hidden UI widget plane (Unreal y 18.24)
SCREEN_MIN_UE_Y = 18.6  # every visible point inside the rectangle must be in front of this

# Body.
BX = 0.227               # bezel half width
BZ0, BZ1 = 0.040, 0.4434  # bezel / front shell bottom and top
YF = -0.2228             # bezel front face (buttons stick out to -0.2244)
SEG = 6                  # segments per rounded corner of the housing rings


# -----------------------------------------------------------------------------
# Small geometry helpers (adapted from RetroComputer/create_retro_computer_blend.py)
# -----------------------------------------------------------------------------


def loft(loops, mat_index, cap_start=True, cap_end=True):
    """Quad strips between equally sized closed loops; optional n-gon caps (closed shell)."""
    bm = bmesh.new()
    rings = [[bm.verts.new(Vector(p)) for p in loop] for loop in loops]
    n = len(rings[0])
    for a, b in zip(rings, rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((a[i], a[j], b[j], b[i]))
    if cap_start:
        bm.faces.new(list(reversed(rings[0])))
    if cap_end:
        bm.faces.new(rings[-1])
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def rr_points(cx, cy, w, h, r, seg):
    """Rounded rectangle outline (2D), 4 * (seg + 1) points, counter-clockwise."""
    hw, hh = w * 0.5, h * 0.5
    r = max(1e-4, min(r, hw - 1e-4, hh - 1e-4))
    pts = []
    for ox, oy, a0 in ((hw - r, hh - r, 0.0), (-hw + r, hh - r, 90.0),
                       (-hw + r, -hh + r, 180.0), (hw - r, -hh + r, 270.0)):
        for k in range(seg + 1):
            a = math.radians(a0 + 90.0 * k / seg)
            pts.append((cx + ox + r * math.cos(a), cy + oy + r * math.sin(a)))
    return pts


def ring(rect, y, seg=SEG):
    """Rounded rectangle loop in the XZ plane at depth y. rect = (x0, x1, z0, z1, corner_r)."""
    x0, x1, z0, z1, r = rect
    return [Vector((p[0], y, p[1])) for p in rr_points((x0 + x1) * 0.5, (z0 + z1) * 0.5,
                                                        x1 - x0, z1 - z0, r, seg)]


def grow(rect, d):
    """Offset a rounded rectangle outward by d (negative = inset), keeping the corners concentric."""
    x0, x1, z0, z1, r = rect
    return (x0 - d, x1 + d, z0 - d, z1 + d, max(r + d, 0.0012))


def ellipse(cx, cy, rx, ry, z, n=36):
    return [Vector((cx + rx * math.cos(2 * math.pi * k / n), cy + ry * math.sin(2 * math.pi * k / n), z))
            for k in range(n)]


def on_plane(bm, origin, angle_x):
    """Place a part built around the origin onto a surface tilted by angle_x around X."""
    return rh.transform(bm, Matrix.Translation(origin) @ Matrix.Rotation(angle_x, 4, "X"))


# -----------------------------------------------------------------------------
# Parts
# -----------------------------------------------------------------------------

OUTER = (-BX, BX, BZ0, BZ1, 0.022)
# Front opening on the bezel face: a chamfered tunnel leads back to the screen rectangle.
OPEN_FRONT = (SCR_X0 - 0.0135, SCR_X1 + 0.0135, SCR_Z0 - 0.0135, SCR_Z1 + 0.0085, 0.012)
OPEN_GLASS = (SCR_X0, SCR_X1, SCR_Z0, SCR_Z1, 0.003)

# Rear shell rings (rect, y): top stays nearly level, bottom rises and sides taper toward the back.
# Top and side vents sit on the flat faces between REAR[1] and REAR[2].
REAR = [
    ((-0.2235, 0.2235, 0.0415, 0.4400, 0.024), -0.1300),
    ((-0.2190, 0.2190, 0.0460, 0.4360, 0.030), -0.0400),
    ((-0.2050, 0.2050, 0.0650, 0.4260, 0.040), 0.0700),
    ((-0.1880, 0.1880, 0.0880, 0.4140, 0.050), 0.1600),
    ((-0.1780, 0.1780, 0.1000, 0.4060, 0.050), 0.2000),
    ((-0.1680, 0.1680, 0.1100, 0.3960, 0.045), 0.2160),   # rounded back edge
    ((-0.1550, 0.1550, 0.1220, 0.3840, 0.035), 0.2241),   # back face (Unreal y -22.41)
]


def build_housing():
    """One closed shell: tunnel -> bezel face -> front shell -> seam groove -> tapered rear shell -> back."""
    loops = [
        ring(grow(OPEN_GLASS, -0.0015), -0.1860),     # tunnel end, behind the glass (capped)
        ring(grow(OPEN_GLASS, -0.0003), -0.1915),     # meets the glass edge (glass edge is 0.3-0.6 mm inside)
        ring(OPEN_FRONT, YF + 0.0028),                # chamfered tunnel wall
        ring(grow(OPEN_FRONT, 0.0012), YF + 0.0008),  # rounded lip
        ring(grow(OPEN_FRONT, 0.0028), YF),           # opening edge on the bezel face
    ]
    rad = 0.004  # rounded outer edge of the bezel face
    for k in range(4):
        th = math.radians(90.0 * k / 3)
        loops.append(ring(grow(OUTER, -rad * (1.0 - math.sin(th))), YF + rad * (1.0 - math.cos(th))))
    loops += [
        ring(OUTER, -0.1505),                         # front shell
        ring(grow(OUTER, -0.0035), -0.1500),          # seam groove
        ring(grow(OUTER, -0.0035), -0.1470),
        ring(grow(OUTER, -0.0015), -0.1460),
    ]
    loops += [ring(rect, y) for rect, y in REAR]
    return loft(loops, I_BODY)


def build_glass():
    """Convex CRT glass over exactly the screen rectangle (Screen), closed by hidden dark sides/back."""
    bm = bmesh.new()
    grid = []
    for j in range(GLASS_NZ + 1):
        vv = j / GLASS_NZ * 2.0 - 1.0
        row = []
        for i in range(GLASS_NX + 1):
            uu = i / GLASS_NX * 2.0 - 1.0
            y = GLASS_Y_EDGE - GLASS_BULGE * (1.0 - uu * uu) * (1.0 - vv * vv)
            row.append(bm.verts.new((SCR_X0 + (uu + 1.0) * 0.5 * SCR_W, y, SCR_Z0 + (vv + 1.0) * 0.5 * SCR_H)))
        grid.append(row)
    for j in range(GLASS_NZ):
        for i in range(GLASS_NX):
            f = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
            f.material_index = I_SCREEN
    nx, nz = GLASS_NX, GLASS_NZ
    border = ([grid[0][i] for i in range(nx + 1)] + [grid[j][nx] for j in range(1, nz + 1)]
              + [grid[nz][i] for i in range(nx - 1, -1, -1)] + [grid[j][0] for j in range(nz - 1, 0, -1)])
    backs = [bm.verts.new((v.co.x, GLASS_Y_BACK, v.co.z)) for v in border]
    center = bm.verts.new(((SCR_X0 + SCR_X1) * 0.5, GLASS_Y_BACK, (SCR_Z0 + SCR_Z1) * 0.5))
    n = len(border)
    for k in range(n):
        k2 = (k + 1) % n
        bm.faces.new((border[k], border[k2], backs[k2], backs[k])).material_index = I_DARK
        bm.faces.new((backs[k2], backs[k], center)).material_index = I_DARK  # fan: no degenerate n-gon
    return bm


def build_controls():
    """Buttons on the thicker bottom bezel strip: four small dark buttons, LED, power button on the right."""
    bm = bmesh.new()
    zc = (BZ0 + OPEN_FRONT[2] - 0.0028) * 0.5 + 0.001
    for x in (-0.037, -0.008, 0.021, 0.050):
        rh.merge_into(bm, rh.cylinder(0.0045, 0.0030, (x, YF + 0.0003, zc), "Y", I_DARK, 12))
        rh.merge_into(bm, rh.cylinder(0.0038, 0.0012, (x, YF - 0.0015, zc), "Y", I_DARK, 12))
    # Power LED and power button (dark ring, beige cap with a softened face).
    rh.merge_into(bm, rh.cylinder(0.0022, 0.0022, (0.146, YF + 0.0002, zc), "Y", I_DARK, 12))
    rh.merge_into(bm, rh.cylinder(0.0112, 0.0024, (0.172, YF + 0.0006, zc), "Y", I_DARK, 24))
    rh.merge_into(bm, rh.cylinder(0.0084, 0.0026, (0.172, YF - 0.0002, zc), "Y", I_BODY, 24))
    rh.merge_into(bm, rh.cylinder(0.0084, 0.0012, (0.172, YF - 0.0010, zc), "Y", I_BODY, 24, radius2=0.0074))
    # Recessed-looking blank strip on the left (no text or logo).
    rh.merge_into(bm, rh.beveled_box(-0.172, -0.110, YF - 0.0005, YF + 0.002, zc - 0.0035, zc + 0.0035,
                                     0.0007, I_DARK, 1))
    return bm


def build_vents():
    bm = bmesh.new()
    # Top vents on the sloped rear shell (between REAR[1] and REAR[2]).
    (r1, y1), (r2, y2) = REAR[1], REAR[2]
    t1, t2 = r1[3], r2[3]
    ang = math.atan2(t2 - t1, y2 - y1)
    ym = (y1 + y2) * 0.5
    zm = (t1 + t2) * 0.5
    for i in range(17):
        x = -0.112 + i * 0.014
        slot = rh.box(x - 0.0024, x + 0.0024, -0.042, 0.042, -0.003, 0.0004, I_DARK)
        rh.merge_into(bm, on_plane(slot, Vector((0.0, ym, zm)), ang))
    # Front shell top: one short row near the seam.
    for i in range(13):
        x = -0.090 + i * 0.015
        rh.merge_into(bm, rh.box(x - 0.0022, x + 0.0022, -0.186, -0.162, BZ1 - 0.003, BZ1 + 0.0002, I_DARK))
    # Back face: horizontal slots plus two connector blocks.
    yb = REAR[-1][1]
    for i in range(8):
        z = 0.215 + i * 0.013
        rh.merge_into(bm, rh.box(-0.085, 0.085, yb - 0.003, yb + 0.0003, z - 0.0022, z + 0.0022, I_DARK))
    rh.merge_into(bm, rh.beveled_box(0.040, 0.090, yb - 0.003, yb + 0.0003, 0.140, 0.165, 0.001, I_DARK, 1))
    rh.merge_into(bm, rh.beveled_box(-0.100, -0.060, yb - 0.003, yb + 0.0003, 0.140, 0.160, 0.001, I_DARK, 1))
    # Side vents on the rear shell flanks (short vertical slots on the flat part of each side).
    (s1, sy1), (s2, sy2) = REAR[1], REAR[2]
    for sgn in (-1.0, 1.0):
        xa, xb = sgn * s1[1], sgn * s2[1]
        side_ang = math.atan2(xb - xa, sy2 - sy1)  # taper angle of the side wall
        for i in range(7):
            y = -0.035 + i * 0.012
            t = (y - sy1) / (sy2 - sy1)
            xs = xa + (xb - xa) * t
            slot = rh.box(-0.003, 0.0004, -0.0022, 0.0022, 0.250, 0.330, I_DARK)
            if sgn < 0:
                slot = rh.transform(slot, Matrix.Scale(-1.0, 4, Vector((1.0, 0.0, 0.0))))
            rh.merge_into(bm, rh.transform(slot, Matrix.Translation((xs, y, 0.0))
                                           @ Matrix.Rotation(-side_ang, 4, "Z")))
    return bm


def build_stand():
    """Dark tilt/swivel stand: oval foot plate and a cradle that tucks into the housing bottom."""
    cy = -0.070
    bm = loft([
        ellipse(0.0, cy, 0.151, 0.141, -0.0009),
        ellipse(0.0, cy, 0.155, 0.145, 0.0015),
        ellipse(0.0, cy, 0.155, 0.145, 0.0090),
        ellipse(0.0, cy, 0.151, 0.141, 0.0125),
        ellipse(0.0, cy, 0.143, 0.133, 0.0145),
    ], I_DARK)
    rh.merge_into(bm, loft([
        ellipse(0.0, cy + 0.010, 0.108, 0.092, 0.0110),
        ellipse(0.0, cy + 0.010, 0.106, 0.090, 0.0230),
        ellipse(0.0, cy + 0.010, 0.099, 0.084, 0.0340),
        ellipse(0.0, cy + 0.010, 0.088, 0.074, 0.0450),
        ellipse(0.0, cy + 0.010, 0.080, 0.066, 0.0620),  # top ends inside the housing
    ], I_DARK))
    return bm


def sort_faces_by_material(bm):
    """Rebuild the bmesh with faces ordered by material index (slot order = first-use order)."""
    out = bmesh.new()
    vmap = {v: out.verts.new(v.co) for v in bm.verts}
    for f in sorted(bm.faces, key=lambda f: f.material_index):
        nf = out.faces.new([vmap[v] for v in f.verts])
        nf.material_index = f.material_index
    bm.free()
    return out


def write_screen_uvs(bm):
    """Planar front projection on the Screen faces (see module docstring for the Unreal contract)."""
    uv_layer = bm.loops.layers.uv.verify()
    for f in bm.faces:
        if f.material_index != I_SCREEN:
            continue
        for loop in f.loops:
            co = loop.vert.co
            loop[uv_layer].uv = ((co.x - SCR_X0) / SCR_W, (co.z - SCR_Z0) / SCR_H)


def build_monitor(mats):
    bm = build_housing()
    rh.merge_into(bm, build_glass())
    rh.merge_into(bm, build_controls())
    rh.merge_into(bm, build_vents())
    rh.merge_into(bm, build_stand())
    bm = sort_faces_by_material(bm)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.normal_update()
    rh.box_project_uvs(bm, 1.0)
    write_screen_uvs(bm)
    obj = rh.finish_object(bm, MESH_NAME, mats, keep_uvs=True)
    obj.data.uv_layers[0].name = "UVMap"
    return obj


# -----------------------------------------------------------------------------
# Checks
# -----------------------------------------------------------------------------


def screen_checks(obj):
    """Numeric checks of the screen contract. Returns a dict and prints it."""
    me = obj.data
    slots = [m.name for m in me.materials]
    uv = me.uv_layers.active.data
    scr = [p for p in me.polygons if p.material_index == I_SCREEN]
    us = [uv[li].uv[0] for p in scr for li in p.loop_indices]
    vs = [uv[li].uv[1] for p in scr for li in p.loop_indices]
    uv_err = max(max(abs(uv[li].uv[0] - (me.vertices[me.loops[li].vertex_index].co.x - SCR_X0) / SCR_W),
                     abs(uv[li].uv[1] - (me.vertices[me.loops[li].vertex_index].co.z - SCR_Z0) / SCR_H))
                 for p in scr for li in p.loop_indices)
    sv = {i for p in scr for i in p.vertices}
    ue_y = [-me.vertices[i].co.y * 100.0 for i in sv]
    xs = [me.vertices[i].co.x for i in sv]
    zs = [me.vertices[i].co.z for i in sv]
    first_poly = {}
    for p in me.polygons:
        first_poly.setdefault(p.material_index, p.index)

    # Ray casts from the front (Unreal +Y) over the full screen rectangle.
    bvh = BVHTree.FromPolygons([v.co.copy() for v in me.vertices], [tuple(p.vertices) for p in me.polygons])
    n_x, n_z = 145, 123
    hits = {"screen": 0, "other": 0, "miss": 0}
    inner_bad = 0
    min_hit_ue_y = 1e9
    for j in range(n_z + 1):
        z = SCR_Z0 + 0.0002 + (SCR_H - 0.0004) * j / n_z
        for i in range(n_x + 1):
            x = SCR_X0 + 0.0002 + (SCR_W - 0.0004) * i / n_x
            loc, _, idx, _ = bvh.ray_cast(Vector((x, -1.0, z)), Vector((0.0, 1.0, 0.0)))
            inner = SCR_X0 + 0.004 < x < SCR_X1 - 0.004 and SCR_Z0 + 0.004 < z < SCR_Z1 - 0.004
            if loc is None:
                hits["miss"] += 1
                inner_bad += inner
                continue
            min_hit_ue_y = min(min_hit_ue_y, -loc.y * 100.0)
            if me.polygons[idx].material_index == I_SCREEN:
                hits["screen"] += 1
            else:
                hits["other"] += 1
                inner_bad += inner
    total = sum(hits.values())
    res = {
        "slots": slots,
        "first_face_per_slot": first_poly,
        "screen_faces": len(scr),
        "screen_uv_min": [round(min(us), 5), round(min(vs), 5)],
        "screen_uv_max": [round(max(us), 5), round(max(vs), 5)],
        "screen_uv_max_error": uv_err,
        "screen_x_cm": [round(min(xs) * 100, 3), round(max(xs) * 100, 3)],
        "screen_z_cm": [round(min(zs) * 100, 3), round(max(zs) * 100, 3)],
        "screen_ue_y_cm": [round(min(ue_y), 3), round(max(ue_y), 3)],
        "rays_screen_fraction": round(hits["screen"] / total, 4),
        "rays": hits,
        "rays_inner_not_screen": inner_bad,
        "rays_min_first_hit_ue_y_cm": round(min_hit_ue_y, 3),
    }
    print("SCREEN CHECK:", json.dumps(res))
    assert slots == [d[0] for d in MAT_DEFS], slots
    assert min(ue_y) > SCREEN_MIN_UE_Y and hits["miss"] == 0 and inner_bad == 0
    assert min_hit_ue_y > SCREEN_MIN_UE_Y
    assert uv_err < 1e-4
    return res


def stats(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [-v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    return {"min": [round(min(xs) * 100, 2), round(min(ys) * 100, 2), round(min(zs) * 100, 2)],
            "max": [round(max(xs) * 100, 2), round(max(ys) * 100, 2), round(max(zs) * 100, 2)],
            "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons),
            "slots": [m.name for m in obj.data.materials]}


# -----------------------------------------------------------------------------
# Preview step (runs after save/export; the .blend is not saved again)
# -----------------------------------------------------------------------------


def uv_test_image(size=512):
    """Grid with corner markers: red TL (big), green TR, blue BL, white BR, plus an up arrow."""
    import numpy as np
    a = np.full((size, size, 4), (0.35, 0.35, 0.35, 1.0), dtype=np.float32)  # row 0 = visual top
    step = size // 8
    for k in range(0, size + 1, step):
        a[max(0, k - 2):k + 2, :, :3] = 0.08
        a[:, max(0, k - 2):k + 2, :3] = 0.08
    m = size // 5
    a[6:6 + m, 6:6 + m, :3] = (1.0, 0.05, 0.05)
    s = size // 10
    a[6:6 + s, size - 6 - s:size - 6, :3] = (0.05, 1.0, 0.05)
    a[size - 6 - s:size - 6, 6:6 + s, :3] = (0.1, 0.2, 1.0)
    a[size - 6 - s:size - 6, size - 6 - s:size - 6, :3] = (1.0, 1.0, 1.0)
    c = size // 2
    for r in range(size // 5):  # arrow head pointing up (toward the top of the image)
        a[c - size // 5 + r, c - r // 2:c + r // 2 + 1, :3] = (1.0, 0.9, 0.1)
    a[c:c + size // 5, c - size // 40:c + size // 40, :3] = (1.0, 0.9, 0.1)
    img = bpy.data.images.new("T_ScreenUVTest", size, size, alpha=True)
    img.pixels.foreach_set(np.flipud(a).ravel())  # Blender stores the bottom row first
    img.pack()
    return img


def render_front(obj, out_png, res=768):
    scene = bpy.context.scene
    cam_data = bpy.data.cameras.new("_FrontCam")
    cam_data.type = "ORTHO"
    cam_data.ortho_scale = 0.50
    cam = bpy.data.objects.new("_FrontCam", cam_data)
    scene.collection.objects.link(cam)
    cam.location = (0.0, -1.5, 0.2217)
    cam.rotation_euler = (math.radians(90.0), 0.0, 0.0)  # looks along Blender +Y = at the screen
    prev = scene.camera
    scene.camera = cam
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "FLAT"
    scene.display.shading.color_type = "TEXTURE"
    scene.render.resolution_x = res
    scene.render.resolution_y = res
    scene.render.filepath = str(out_png)
    bpy.ops.render.render(write_still=True)
    scene.display.shading.light = "STUDIO"
    scene.camera = prev
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.cameras.remove(cam_data)


def check_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    out = {}
    for o in new:
        me = o.data
        names = [m.name for m in me.materials]
        si = next(i for i, n in enumerate(names) if n.startswith("Screen"))
        uv = me.uv_layers.active.data
        lis = [li for p in me.polygons if p.material_index == si for li in p.loop_indices]
        ws = [o.matrix_world @ me.vertices[me.loops[li].vertex_index].co for li in lis]
        out[o.name] = {"slots": names, "screen_slot": si,
                       "uv_min": [round(min(uv[li].uv[0] for li in lis), 4), round(min(uv[li].uv[1] for li in lis), 4)],
                       "uv_max": [round(max(uv[li].uv[0] for li in lis), 4), round(max(uv[li].uv[1] for li in lis), 4)],
                       "screen_world_x": [round(min(w.x for w in ws), 4), round(max(w.x for w in ws), 4)],
                       "screen_world_z": [round(min(w.z for w in ws), 4), round(max(w.z for w in ws), 4)]}
    print("FBX CHECK:", json.dumps(out))
    for o in new:
        bpy.data.objects.remove(o, do_unlink=True)
    return out


def preview(obj, preview_dir):
    preview_dir.mkdir(parents=True, exist_ok=True)
    rh.preview_render(obj, preview_dir / f"{MESH_NAME}.png")
    rh.preview_render(obj, preview_dir / f"{MESH_NAME}_rear.png", azimuth_deg=-125.0, elevation_deg=25.0)
    rh.preview_render(obj, preview_dir / f"{MESH_NAME}_side.png", azimuth_deg=10.0, elevation_deg=8.0)
    # UV test: test grid on the Screen material, front view (red marker must be top-left, arrow up).
    img = uv_test_image()
    rh.save_image(img, preview_dir / "T_ScreenUVTest.png")
    mat = obj.data.materials[I_SCREEN]
    tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = img
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    mat.node_tree.nodes.active = tex
    render_front(obj, preview_dir / f"{MESH_NAME}_ScreenUV.png")
    rh.preview_render(obj, preview_dir / f"{MESH_NAME}_ScreenUV_persp.png", azimuth_deg=70.0, elevation_deg=12.0)
    info = {"stats": stats(obj), "screen": screen_checks(obj), "fbx": check_fbx(OUTPUT_FBX)}
    (preview_dir / "stats.json").write_text(json.dumps(info, indent=1))


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview_dir = Path(args[args.index("--preview") + 1]) if "--preview" in args else None

    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for CRT_VCR_Material Monitor_r1 (the player's interactive CRT).",
        Pivot="Bottom center of the stand. Screen faces Unreal +Y (= Blender -Y).",
        ScreenContract="Slot 1 = Screen; screen x -18.095..18.095 cm, z 11.015..41.665 cm; planar UVs 0..1.",
        ExportFBX=str(OUTPUT_FBX))
    mats = [rh.make_material(name, c, m, r) for name, c, m, r in MAT_DEFS]
    obj = build_monitor(mats)
    rh.report(obj)
    screen_checks(obj)

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, OUTPUT_FBX)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")
    print(f"Exported FBX: {OUTPUT_FBX}")

    if preview_dir is not None:
        preview(obj, preview_dir)


if __name__ == "__main__":
    main()
