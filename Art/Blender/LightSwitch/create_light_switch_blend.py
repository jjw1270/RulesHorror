import math
import sys
from pathlib import Path

import bmesh
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready wall switch combination (two stacked wall plates).
# Replaces /Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/SM_Light_Switch3 (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = center of the lower plate on the wall plane, plates stick out toward Unreal +Y
#   (Blender -Y), Z up, meters. Upper plate: two rocker switches. Lower plate: socket outlet.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "LightSwitch.blend"
OUTPUT_FBX = BASE_DIR / "Exports" / "SM_LightSwitch.fbx"
MESH_NAME = "SM_LightSwitch"

MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Plastic_Plate", (0.60, 0.60, 0.56, 1.0), 0.0, 0.38),
    ("Plastic_Insert", (0.68, 0.68, 0.65, 1.0), 0.0, 0.30),
    ("Socket_Contact", (0.012, 0.012, 0.012, 1.0), 0.0, 0.6),
]
I_PLATE, I_INSERT, I_DARK = 0, 1, 2

CM = 0.01
# Depth d is measured in Unreal +Y from the wall plane (Blender y = -d).
D_BACK = -0.4 * CM     # plates sink 4 mm into the wall like the original
D_FRAME = 2.4 * CM     # front of the frames
D_BASE = 1.5 * CM      # mechanism face inside the frame opening
X_MIN, X_MAX = -6.3 * CM, 6.4 * CM
CX = (X_MIN + X_MAX) * 0.5
HW = (X_MAX - X_MIN) * 0.5
LOWER = (-6.3 * CM, 6.3 * CM)
UPPER = (7.25 * CM, 19.8 * CM)


# -----------------------------------------------------------------------------
# Geometry helpers (X-Z profiles extruded along depth)
# -----------------------------------------------------------------------------


def rounded_rect_loop(cx, cz, hw, hh, r, k=4):
    """Counter-clockwise (x, z) points of a rounded rectangle, 4 * (k + 1) points."""
    pts = []
    corners = [(cx + hw - r, cz + hh - r, 0.0), (cx - hw + r, cz + hh - r, 90.0),
               (cx - hw + r, cz - hh + r, 180.0), (cx + hw - r, cz - hh + r, 270.0)]
    for ccx, ccz, a0 in corners:
        for i in range(k + 1):
            a = math.radians(a0 + 90.0 * i / k)
            pts.append((ccx + r * math.cos(a), ccz + r * math.sin(a)))
    return pts


def ring_prism(outer, inner, d0, d1, mat_index, front_support=()):
    """Frame with an opening: outer/inner loops (same point count), from depth d0 (back) to d1 (front).
    front_support: extra loops (outer to inner order) on the flat front face; they keep bevel shading
    from smearing across the face. Returns (bm, front_outer_edges, front_inner_edges)."""
    bm = bmesh.new()
    n = len(outer)
    front = [[bm.verts.new((x, -d1, z)) for x, z in loop] for loop in (outer, *front_support, inner)]
    of, inf = front[0], front[-1]
    ob = [bm.verts.new((x, -d0, z)) for x, z in outer]
    inb = [bm.verts.new((x, -d0, z)) for x, z in inner]
    for i in range(n):
        j = (i + 1) % n
        for a, b in zip(front[:-1], front[1:]):
            bm.faces.new((a[i], a[j], b[j], b[i]))
        bm.faces.new((of[i], ob[i], ob[j], of[j]))
        bm.faces.new((inf[i], inf[j], inb[j], inb[i]))
        bm.faces.new((ob[i], inb[i], inb[j], ob[j]))
    for f in bm.faces:
        f.material_index = mat_index
    bm.edges.ensure_lookup_table()
    fo = [bm.edges.get((of[i], of[(i + 1) % n])) for i in range(n)]
    fi = [bm.edges.get((inf[i], inf[(i + 1) % n])) for i in range(n)]
    return bm, fo, fi


def plate(x0, x1, z0, z1, d0, d1, mat_index, bevel=0.0, segments=2):
    """Axis-aligned block given in Unreal-style depth (d0 back, d1 front)."""
    bm = rh.box(x0, x1, -d1, -d0, z0, z1, mat_index)
    rh.bevel_edges(bm, bm.edges, bevel, segments)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def rocker(cx, cz, w, h, d0, d1, tilt, mat_index):
    """Rocker switch: block whose front face is tilted (tilt > 0: top sticks out)."""
    bm = rh.box(cx - w * 0.5, cx + w * 0.5, -d1, -d0, cz - h * 0.5, cz + h * 0.5, mat_index)
    for v in bm.verts:
        if -v.co.y > (d0 + d1) * 0.5:  # front verts
            v.co.y -= tilt * (v.co.z - cz) / (h * 0.5)
    rh.bevel_edges(bm, bm.edges, 0.0016, 2)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def socket_insert(cx, cz, hw, d0, d1, hole_r, hole_depth, mat_index, n=32):
    """Square insert with a round socket recess (front face ring, recess wall, recess floor)."""
    bm = bmesh.new()
    sq_f, sq_b, c_f, c_b = [], [], [], []
    m_out, m_in = [], []  # support loops keep the bevel shading off the flat front face
    for i in range(n):
        a = 2.0 * math.pi * i / n + math.pi / 4.0
        ca, sa = math.cos(a), math.sin(a)
        s = min(hw / max(abs(ca), 1e-9), hw / max(abs(sa), 1e-9))
        sx, sz = cx + ca * s, cz + sa * s
        sq_f.append(bm.verts.new((sx, -d1, sz)))
        sq_b.append(bm.verts.new((sx, -d0, sz)))
        m_out.append(bm.verts.new((cx + ca * (s - 0.004), -d1, cz + sa * (s - 0.004))))
        m_in.append(bm.verts.new((cx + ca * (hole_r + 0.002), -d1, cz + sa * (hole_r + 0.002))))
        c_f.append(bm.verts.new((cx + ca * hole_r, -d1, cz + sa * hole_r)))
        c_b.append(bm.verts.new((cx + ca * hole_r, -(d1 - hole_depth), cz + sa * hole_r)))
    back_c = bm.verts.new((cx, -d0, cz))
    floor_c = bm.verts.new((cx, -(d1 - hole_depth), cz))
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((sq_f[i], sq_f[j], m_out[j], m_out[i]))    # front ring (outer strip)
        bm.faces.new((m_out[i], m_out[j], m_in[j], m_in[i]))    # front ring
        bm.faces.new((m_in[i], m_in[j], c_f[j], c_f[i]))        # front ring (inner strip)
        bm.faces.new((sq_f[i], sq_b[i], sq_b[j], sq_f[j]))      # outer wall
        bm.faces.new((c_f[i], c_f[j], c_b[j], c_b[i]))          # recess wall
        bm.faces.new((c_b[i], c_b[j], floor_c))                 # recess floor
        bm.faces.new((sq_b[j], sq_b[i], back_c))                # back
    for f in bm.faces:
        f.material_index = mat_index
    bm.edges.ensure_lookup_table()
    front_outer = [bm.edges.get((sq_f[i], sq_f[(i + 1) % n])) for i in range(n)]
    front_inner = [bm.edges.get((c_f[i], c_f[(i + 1) % n])) for i in range(n)]
    rh.bevel_edges(bm, front_outer, 0.0018, 2)
    rh.bevel_edges(bm, [e for e in front_inner if e.is_valid], 0.0008, 1)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def wall_plate(z0, z1, opening_hw, opening_hh):
    """Rounded frame with a rectangular opening plus the mechanism base filling the opening."""
    cz = (z0 + z1) * 0.5
    hh = (z1 - z0) * 0.5
    outer = rounded_rect_loop(CX, cz, HW, hh, 0.9 * CM)
    inner = rounded_rect_loop(CX, cz, opening_hw, opening_hh, 0.25 * CM)
    support = (rounded_rect_loop(CX, cz, HW - 0.75 * CM, hh - 0.75 * CM, 0.6 * CM),
               rounded_rect_loop(CX, cz, opening_hw + 0.25 * CM, opening_hh + 0.25 * CM, 0.4 * CM))
    bm, fo, fi = ring_prism(outer, inner, D_BACK, D_FRAME, I_PLATE, support)
    rh.bevel_edges(bm, fo, 0.55 * CM, 3)
    rh.bevel_edges(bm, [e for e in fi if e.is_valid], 0.12 * CM, 1)
    for f in bm.faces:
        f.material_index = I_PLATE
    # Mechanism base, slightly larger than the opening so no crack shows.
    rh.merge_into(bm, plate(CX - opening_hw - 0.001, CX + opening_hw + 0.001,
                            cz - opening_hh - 0.001, cz + opening_hh + 0.001,
                            0.0, D_BASE, I_INSERT))
    return bm


def create_mesh(mats):
    bm = bmesh.new()

    # Upper plate: two rocker switches.
    ucz = (UPPER[0] + UPPER[1]) * 0.5
    u_hw, u_hh = 4.35 * CM, 4.3 * CM
    rh.merge_into(bm, wall_plate(UPPER[0], UPPER[1], u_hw, u_hh))
    rw, gap = 4.15 * CM, 0.2 * CM
    for side, tilt in ((-1, 0.55 * CM), (1, -0.45 * CM)):
        rx = CX + side * (rw + gap) * 0.5
        rh.merge_into(bm, rocker(rx, ucz, rw, 8.25 * CM, D_BASE - 0.002, 3.35 * CM, tilt, I_INSERT))

    # Lower plate: square insert with a round socket outlet.
    l_hw = 4.6 * CM
    rh.merge_into(bm, wall_plate(LOWER[0], LOWER[1], l_hw, l_hw))
    rh.merge_into(bm, socket_insert(CX, 0.0, 4.45 * CM, D_BASE - 0.002, 3.3 * CM,
                                    1.95 * CM, 1.3 * CM, I_INSERT))
    # Pin holes (dark discs on the recess floor).
    floor_d = 3.3 * CM - 1.3 * CM
    for sx in (-1, 1):
        rh.merge_into(bm, rh.cylinder(0.25 * CM, 0.06 * CM, (CX + sx * 0.95 * CM, -(floor_d + 0.02 * CM), 0.0),
                                      axis="Y", mat_index=I_DARK, segments=12))
    # Earthing clips at the top and bottom of the recess wall.
    for sz in (-1, 1):
        zc = sz * 1.82 * CM
        rh.merge_into(bm, plate(CX - 0.55 * CM, CX + 0.55 * CM, zc - 0.15 * CM, zc + 0.15 * CM,
                                floor_d, floor_d + 0.9 * CM, I_DARK, bevel=0.0004, segments=1))

    obj = rh.finish_object(bm, MESH_NAME, mats, sharp_angle_deg=40.0)
    return obj


def main():
    rh.clear_scene()
    rh.set_scene(AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Light_Switch3 (wall switch + socket).",
                 Pivot="Center of the lower plate on the wall plane. Plates face Unreal +Y (Blender -Y), Z up.",
                 ExportFBX=str(OUTPUT_FBX))
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    obj = create_mesh(mats)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, OUTPUT_FBX)
    rh.report(obj)
    for p in BASE_DIR.glob("*.blend1"):
        p.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")
    print(f"Exported FBX: {OUTPUT_FBX}")


if __name__ == "__main__":
    main()
