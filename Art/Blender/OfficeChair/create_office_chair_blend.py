import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready swivel office chair.
# Replaces BrutalistLevelKit SM_Office_Chair (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = floor level under the gas lift. The seat faces Unreal +Y (Blender -Y), the backrest
#   is on the Unreal -Y side. One leg of the 5-star base points backwards (Unreal -Y), which is
#   what makes the original bounds asymmetric in Y (-41.3 .. 32.4 cm). Z up, meters.
#
# This script works directly in Blender coordinates (meters): FRONT is -Y, BACK is +Y.
#
# Optional preview render (not saved into the .blend):
#   blender --background --factory-startup --python create_office_chair_blend.py -- --preview <dir>

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "OfficeChair.blend"
EXPORT_DIR = BASE_DIR / "Exports"

MESH_NAME = "SM_OfficeChair"

# Material slots (slot names in Unreal come from these names).
MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Fabric_Green", (0.09, 0.2, 0.14, 1.0), 0.0, 0.9),
    ("Plastic_Black", (0.02, 0.02, 0.022, 1.0), 0.0, 0.45),
    ("Metal_Chrome", (0.6, 0.6, 0.62, 1.0), 1.0, 0.3),
]
I_FABRIC, I_PLASTIC, I_CHROME = 0, 1, 2

# Base.
LEG_COUNT = 5
LEG_BACK_ANGLE = 90.0           # degrees from +X; +Y Blender = Unreal -Y (backwards)
CASTER_R = 0.345                # caster center radius
WHEEL_R, WHEEL_W = 0.034, 0.016
HUB_R = 0.062
# Seat.
SEAT_W, SEAT_D = 0.54, 0.51
SEAT_CY = -0.035                # seat center (slightly forward of the gas lift)
SEAT_Z0, SEAT_Z1 = 0.452, 0.535
SHELL_Z0 = 0.418
# Backrest (built in its own frame, then reclined around its bottom edge).
BACK_PIVOT = Vector((0.0, 0.235, 0.69))
BACK_RECLINE = 13.0             # degrees
BACK_H = 0.612
BACK_W0, BACK_W1 = 0.48, 0.40
CUSHION_T, SHELL_T, RIM = 0.05, 0.035, 0.02


# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------


def set_mat(bm, mat):
    for f in bm.faces:
        f.material_index = mat
    return bm


def rounded_outline(w0, w1, h, r_bottom, r_top, seg=6):
    """Closed outline (x, z) of a tapered rectangle with rounded corners, z from 0 to h."""
    pts = []

    def arc(cx, cz, r, a0, a1):
        for i in range(seg + 1):
            a = math.radians(a0 + (a1 - a0) * i / seg)
            pts.append((cx + r * math.cos(a), cz + r * math.sin(a)))

    arc(w0 / 2 - r_bottom, r_bottom, r_bottom, -90, 0)
    arc(w1 / 2 - r_top, h - r_top, r_top, 0, 90)
    arc(-w1 / 2 + r_top, h - r_top, r_top, 90, 180)
    arc(-w0 / 2 + r_bottom, r_bottom, r_bottom, 180, 270)
    return pts


def slab(outline, y0, y1, edge_r, mat, seg=2):
    """Extrude an (x, z) outline between y0 and y1 and soften the outline edges of both caps."""
    bm = bmesh.new()
    front = [bm.verts.new((x, y0, z)) for x, z in outline]
    back = [bm.verts.new((x, y1, z)) for x, z in outline]
    bm.faces.new(front)
    bm.faces.new(list(reversed(back)))
    n = len(outline)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((front[j], front[i], back[i], back[j]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    caps = [f for f in bm.faces if len(f.verts) > 4]
    rh.bevel_edges(bm, {e for f in caps for e in f.edges}, edge_r, seg)
    return set_mat(bm, mat)


def rot_z(deg):
    return Matrix.Rotation(math.radians(deg), 4, "Z")


def back_frame():
    """Backrest local frame -> world: recline around the bottom edge, top leaning back (+Y)."""
    return Matrix.Translation(BACK_PIVOT) @ Matrix.Rotation(math.radians(-BACK_RECLINE), 4, "X")


# -----------------------------------------------------------------------------
# Parts
# -----------------------------------------------------------------------------


def leg(angle_deg):
    """Tapered chrome leg from the hub to the caster socket, sloping down toward the tip."""
    r0, r1 = 0.035, CASTER_R + 0.004
    bm = rh.box(r0, r1, -0.031, 0.031, -0.025, 0.025, I_CHROME)
    for v in bm.verts:
        t = (v.co.x - r0) / (r1 - r0)
        v.co.y *= 1.0 - 0.42 * t
        v.co.z *= 1.0 - 0.4 * t
    rh.bevel_edges(bm, bm.edges, 0.008, 2)
    for v in bm.verts:
        t = (v.co.x - r0) / (r1 - r0)
        v.co.z += 0.162 - 0.055 * t
    set_mat(bm, I_CHROME)
    return rh.transform(bm, rot_z(angle_deg))


def caster(angle_deg):
    """Twin-wheel caster: socket, chrome hood and two black wheels rolling along the leg."""
    bm = bmesh.new()
    tip = CASTER_R
    rh.merge_into(bm, rh.cylinder(0.011, 0.03, (tip, 0.0, 0.1), "Z", I_CHROME, 10))
    hood = rh.rounded_box(0.07, 0.064, 0.036, 0.1, 0.022, 0.014, I_CHROME, center=(tip + 0.006, 0.0))
    rh.merge_into(bm, hood)
    for side in (-1.0, 1.0):
        rh.merge_into(bm, rh.cylinder(WHEEL_R, WHEEL_W, (tip + 0.006, side * (0.0175 + WHEEL_W / 2), WHEEL_R),
                                      "Y", I_PLASTIC, 16))
    return rh.transform(bm, rot_z(angle_deg))


def base(bm):
    for k in range(LEG_COUNT):
        a = LEG_BACK_ANGLE + k * 360.0 / LEG_COUNT
        rh.merge_into(bm, leg(a))
        rh.merge_into(bm, caster(a))
    rh.merge_into(bm, rh.cylinder(HUB_R, 0.06, (0.0, 0.0, 0.15), "Z", I_CHROME, 20))
    rh.merge_into(bm, rh.cylinder(HUB_R - 0.003, 0.0145, (0.0, 0.0, 0.1865), "Z", I_CHROME, 20, radius2=HUB_R - 0.018))
    # Gas lift: black telescoping shroud and chrome piston.
    rh.merge_into(bm, rh.cylinder(0.034, 0.15, (0.0, 0.0, 0.265), "Z", I_PLASTIC, 16))
    rh.merge_into(bm, rh.cylinder(0.019, 0.075, (0.0, 0.0, 0.365), "Z", I_CHROME, 12))


def seat(bm):
    # Tilt mechanism, height lever and seat plate.
    rh.merge_into(bm, rh.beveled_box(-0.09, 0.09, -0.11, 0.09, 0.385, 0.432, 0.006, I_PLASTIC, 1))
    rh.merge_into(bm, rh.cylinder(0.005, 0.13, (0.15, -0.07, 0.405), "X", I_CHROME, 8))
    rh.merge_into(bm, rh.cylinder(0.011, 0.04, (0.225, -0.07, 0.405), "X", I_PLASTIC, 10))
    # Black shell under the cushion.
    rh.merge_into(bm, rh.rounded_box(SEAT_W + 0.024, SEAT_D + 0.024, SHELL_Z0, SEAT_Z0 + 0.02, 0.1, 0.01,
                                     I_PLASTIC, center=(0.0, SEAT_CY)))
    # Fabric cushion with a soft waterfall front edge.
    cushion = rh.rounded_box(SEAT_W, SEAT_D, SEAT_Z0, SEAT_Z1, 0.095, 0.03, I_FABRIC,
                             center=(0.0, SEAT_CY), edge_segments=3)
    front_y = SEAT_CY - SEAT_D / 2
    for v in cushion.verts:
        if v.co.z > SEAT_Z0 + 0.03:
            t = max(0.0, min(1.0, (front_y + 0.16 - v.co.y) / 0.16))
            v.co.z -= 0.012 * t * t
    rh.merge_into(bm, cushion)


def backrest(bm):
    frame = back_frame()
    # Black shell (slightly larger outline, shows as a rim around the fabric) and fabric cushion.
    shell = slab(rounded_outline(BACK_W0 + 2 * RIM, BACK_W1 + 2 * RIM, BACK_H + RIM, 0.05, 0.14),
                 -0.014, SHELL_T, 0.008, I_PLASTIC, 2)
    rh.transform(shell, Matrix.Translation((0.0, 0.0, -RIM * 0.5)))
    cushion = slab(rounded_outline(BACK_W0, BACK_W1, BACK_H, 0.04, 0.125),
                   -CUSHION_T, -0.004, 0.016, I_FABRIC, 3)
    # Gentle lumbar curve: push the middle of the cushion front forward a little.
    for v in cushion.verts:
        if v.co.y < -0.03:
            t = v.co.z / BACK_H
            v.co.y -= 0.012 * math.sin(math.pi * min(1.0, max(0.0, (t - 0.05) / 0.6)))
    # Spine behind the backrest, down to the seat level.
    spine = rh.beveled_box(-0.032, 0.032, SHELL_T - 0.004, SHELL_T + 0.026, -0.29, 0.34, 0.006, I_PLASTIC, 1)
    # Lumbar bracket under the backrest with a small fabric pad.
    bracket = rh.beveled_box(-0.2, 0.2, -0.012, 0.03, -0.1, -0.012, 0.006, I_PLASTIC, 1)
    pad = rh.beveled_box(-0.07, 0.07, -0.026, -0.008, -0.088, -0.024, 0.005, I_FABRIC, 2)
    for part in (shell, cushion, spine, bracket, pad):
        rh.merge_into(bm, rh.transform(part, frame))
    # Horizontal spine bar from the mechanism back to the reclined spine.
    rh.merge_into(bm, rh.beveled_box(-0.032, 0.032, 0.05, 0.27, 0.392, 0.428, 0.006, I_PLASTIC, 1))


def armrests(bm):
    for sx in (-1.0, 1.0):
        def xr(a, b):
            return (min(sx * a, sx * b), max(sx * a, sx * b))
        # Under-seat bracket (flat chrome).
        rh.merge_into(bm, rh.beveled_box(*xr(0.075, 0.322), 0.04, 0.12, 0.398, 0.424, 0.005, I_CHROME, 1))
        # Upright, bar and black pad, turned slightly outward at the front.
        arm = bmesh.new()
        rh.merge_into(arm, rh.beveled_box(*xr(0.292, 0.322), 0.04, 0.12, 0.398, 0.668, 0.005, I_CHROME, 1))
        rh.merge_into(arm, rh.beveled_box(*xr(0.2935, 0.3205), -0.1, 0.115, 0.643, 0.6665, 0.005, I_CHROME, 1))
        rh.merge_into(arm, rh.rounded_box(0.072, 0.28, 0.668, 0.701, 0.024, 0.009, I_PLASTIC,
                                          center=(sx * 0.307, -0.035)))
        pivot = Vector((sx * 0.307, 0.08, 0.0))
        rh.transform(arm, Matrix.Translation(pivot) @ rot_z(sx * 7.0) @ Matrix.Translation(-pivot))
        rh.merge_into(bm, arm)


def build_chair():
    bm = bmesh.new()
    base(bm)
    seat(bm)
    backrest(bm)
    armrests(bm)
    return bm


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def preview_dir():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--preview" in argv:
        return Path(argv[argv.index("--preview") + 1])
    return None


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Office_Chair (swivel office chair).",
        Pivot="Floor level under the gas lift. Seat faces Blender -Y (Unreal +Y), backrest on Blender +Y.",
    )
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    obj = rh.finish_object(build_chair(), MESH_NAME, mats, uv_scale=1.0, sharp_angle_deg=40.0)

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, EXPORT_DIR / f"{MESH_NAME}.fbx")
    rh.report(obj)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()

    out = preview_dir()
    if out is not None:
        rh.preview_render(obj, out / f"{MESH_NAME}.png")
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
