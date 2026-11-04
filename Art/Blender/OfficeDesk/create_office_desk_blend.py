import sys
from pathlib import Path

import bmesh
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready worn wooden office desks (two variants).
#   SM_OfficeDesk_A replaces BrutalistLevelKit SM_Office_Table1_4X
#     (narrow left pedestal + wide right pedestal, open knee space).
#   SM_OfficeDesk_B replaces BrutalistLevelKit SM_Office_Table2_4X
#     (two narrow pedestals + a center drawer under the top).
# Same bounds and pivot as the originals so placed actors keep their transform:
#   pivot = back edge of the top (Unreal y = 0), centered in X, floor at z = 0.
#   Drawer fronts face Unreal +Y (Blender -Y). Z up, meters.
#
# All layout numbers below are written in Unreal centimeters (x, y, z) and
# converted with ue_box(); Blender y = -Unreal y.
#
# Optional preview renders (not saved into the .blend):
#   blender --background --factory-startup --python create_office_desk_blend.py -- --preview <dir>

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "OfficeDesk.blend"
EXPORT_DIR = BASE_DIR / "Exports"

NAME_A = "SM_OfficeDesk_A"
NAME_B = "SM_OfficeDesk_B"

# Material slots (slot names in Unreal come from these names).
MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Wood_Top", (0.30, 0.235, 0.17, 1.0), 0.0, 0.7),     # worn, sun-bleached top
    ("Wood", (0.26, 0.17, 0.10, 1.0), 0.0, 0.65),         # drawer fronts
    ("Frame", (0.10, 0.08, 0.065, 1.0), 0.0, 0.6),
    ("Metal_Handles", (0.6, 0.6, 0.62, 1.0), 1.0, 0.3),
]
I_TOP, I_WOOD, I_FRAME, I_METAL = 0, 1, 2, 3

# Layout (Unreal cm).
X0, X1 = -99.8, 99.75          # top extents
Y0, Y1 = 0.9, 101.35           # back / front edge of the top
TOP_Z0, TOP_Z1 = 78.5, 82.95   # top slab
END_L = (-97.0, -94.5)         # left end panel (outer side of left pedestal), 2.5 cm thick
END_R = (94.5, 97.0)           # right end panel
PANEL_Y0, FRONT_Y = 2.5, 98.5  # side panels run from the back to the front plane
DRAWER_T = 2.0                 # drawer fronts sit in front of the carcass
CORE_Y0 = 3.0
CORE_Z0 = 8.0                  # pedestal carcass bottom (feet underneath)
DRAWER_Z0, DRAWER_Z1 = 11.0, 77.6
GAP = 0.5
FOOT_Z0 = -0.65


def ue_box(x0, x1, y0, y1, z0, z1):
    """Unreal cm box extents -> Blender meter extents (x0, x1, y0, y1, z0, z1)."""
    return (x0 / 100.0, x1 / 100.0, -y1 / 100.0, -y0 / 100.0, z0 / 100.0, z1 / 100.0)


def part(bm, x0, x1, y0, y1, z0, z1, mat, bevel=0.0, seg=2):
    c = ue_box(x0, x1, y0, y1, z0, z1)
    p = rh.beveled_box(*c, bevel / 100.0, mat, seg) if bevel > 0 else rh.box(*c, mat)
    rh.merge_into(bm, p)


def cyl(bm, radius, z0, z1, x, y, mat, segments=12):
    rh.merge_into(bm, rh.cylinder(radius / 100.0, (z1 - z0) / 100.0,
                                  (x / 100.0, -y / 100.0, (z0 + z1) / 200.0), "Z", mat, segments))


# -----------------------------------------------------------------------------
# Parts
# -----------------------------------------------------------------------------


def handle(bm, cx, cz, ajar=0.0):
    """Loop pull: two mounting plates, two arms and a front bar, on a drawer front at y = FRONT_Y + ajar."""
    fy = FRONT_Y + ajar
    for sx in (-6.0, 6.0):
        part(bm, cx + sx - 1.1, cx + sx + 1.1, fy - 0.1, fy + 0.3, cz - 1.4, cz + 1.4, I_METAL, 0.12, 1)
        part(bm, cx + sx - 0.55, cx + sx + 0.55, fy + 0.2, fy + 2.0, cz - 0.55, cz + 0.55, I_METAL)
    part(bm, cx - 6.8, cx + 6.8, fy + 1.75, fy + 2.75, cz - 0.6, cz + 0.6, I_METAL, 0.25, 1)


def drawer(bm, x0, x1, z0, z1, handle_from_top=True, ajar=0.0):
    fy = FRONT_Y + ajar
    part(bm, x0, x1, fy - DRAWER_T, fy, z0, z1, I_WOOD, 0.55, 2)
    hz = z1 - min(10.0, (z1 - z0) * 0.5) if handle_from_top else (z0 + z1) * 0.5
    handle(bm, (x0 + x1) * 0.5, hz, ajar)


def foot(bm, x, y):
    cyl(bm, 2.8, FOOT_Z0, 1.4, x, y, I_METAL, 12)
    cyl(bm, 1.1, 1.4, CORE_Z0 + 0.5, x, y, I_METAL, 8)


def pedestal(bm, x_in0, x_in1, drawers, ajar_index=None):
    """Pedestal between two side-panel inner faces x_in0..x_in1 (the side panels are built separately)."""
    # Carcass core (closed box: back, bottom and the shadowed reveal around the drawers).
    part(bm, x_in0, x_in1, CORE_Y0, FRONT_Y - DRAWER_T, CORE_Z0, TOP_Z0, I_FRAME)
    # Front kick rail below the bottom drawer.
    part(bm, x_in0, x_in1, FRONT_Y - DRAWER_T - 1.0, FRONT_Y - 0.6, CORE_Z0, DRAWER_Z0 - GAP, I_FRAME, 0.3, 1)
    h = (DRAWER_Z1 - DRAWER_Z0 - GAP * (drawers - 1)) / drawers
    for i in range(drawers):
        z1 = DRAWER_Z1 - i * (h + GAP)
        drawer(bm, x_in0 + 0.4, x_in1 - 0.4, z1 - h, z1, True, 1.0 if i == ajar_index else 0.0)
    cx = (x_in0 + x_in1) * 0.5
    foot(bm, cx, FRONT_Y - 9.0)
    foot(bm, cx, CORE_Y0 + 8.0)


def side_panel(bm, x0, x1, z0):
    part(bm, x0, x1, PANEL_Y0, FRONT_Y, z0, TOP_Z0, I_FRAME, 0.4, 2)


def build_desk(variant):
    bm = bmesh.new()

    # Worn top slab with softened edges.
    part(bm, X0, X1, Y0, Y1, TOP_Z0, TOP_Z1, I_TOP, 0.8, 3)

    # End panels reach the floor; they are the outer sides of the two pedestals.
    side_panel(bm, *END_L, 0.0)
    side_panel(bm, *END_R, 0.0)

    if variant == "A":
        # Narrow left pedestal (2 drawers), wide right pedestal (2 drawers), open knee space.
        left_inner = (-52.0, -49.5)
        right_inner = (12.5, 15.0)
        side_panel(bm, *left_inner, CORE_Z0 - 1.0)
        side_panel(bm, *right_inner, CORE_Z0 - 1.0)
        pedestal(bm, END_L[1], left_inner[0], 2)
        pedestal(bm, right_inner[1], END_R[0], 2, ajar_index=1)
        knee = (left_inner[1], right_inner[0])
        # Slim rail under the front edge of the top.
        part(bm, knee[0], knee[1], FRONT_Y - 5.0, FRONT_Y - 2.5, TOP_Z0 - 3.0, TOP_Z0, I_FRAME, 0.3, 1)
    else:
        # Two equal pedestals (2 drawers each) and a center drawer under the top.
        left_inner = (-52.0, -49.5)
        right_inner = (49.5, 52.0)
        side_panel(bm, *left_inner, CORE_Z0 - 1.0)
        side_panel(bm, *right_inner, CORE_Z0 - 1.0)
        pedestal(bm, END_L[1], left_inner[0], 2, ajar_index=0)
        pedestal(bm, right_inner[1], END_R[0], 2)
        knee = (left_inner[1], right_inner[0])
        # Center drawer housing and front.
        cz0 = 64.0
        part(bm, knee[0], knee[1], 45.0, FRONT_Y - DRAWER_T, cz0 - 1.5, TOP_Z0, I_FRAME)
        drawer(bm, knee[0] + 0.4, knee[1] - 0.4, cz0, DRAWER_Z1, handle_from_top=False)

    # Modesty panel at the back of the knee space.
    part(bm, knee[0], knee[1], 3.5, 5.5, 28.0, TOP_Z0, I_FRAME, 0.3, 1)
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
        AssetPurpose="Self-made replacements for BrutalistLevelKit SM_Office_Table1_4X / SM_Office_Table2_4X.",
        Pivot="Back edge of the top, centered in X, floor at z=0. Drawer fronts face Blender -Y (Unreal +Y).",
    )
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]

    objs = []
    for name, variant in ((NAME_A, "A"), (NAME_B, "B")):
        objs.append(rh.finish_object(build_desk(variant), name, mats, uv_scale=1.0, sharp_angle_deg=35.0))

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    for obj in objs:
        rh.export_fbx(obj, EXPORT_DIR / f"{obj.name}.fbx")
        rh.report(obj)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()

    out = preview_dir()
    if out is not None:
        for obj in objs:
            for o in objs:
                o.hide_render = o is not obj
            rh.preview_render(obj, out / f"{obj.name}.png")
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
