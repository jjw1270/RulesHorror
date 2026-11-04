import sys
from pathlib import Path

import bmesh
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready office cubicle partition.
# Replaces BrutalistLevelKit SM_Desk_Partition_4X (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = floor level, center of the thickness (y = 0), 0.5 cm left of the width center
#   (the original spans Unreal x -100.5 .. 99.5). Both faces are identical. Z up, meters.
#
# Layout numbers are written in Unreal centimeters and converted with ue_box(); Blender y = -Unreal y.
#
# Optional preview render (not saved into the .blend):
#   blender --background --factory-startup --python create_desk_partition_blend.py -- --preview <dir>

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "DeskPartition.blend"
EXPORT_DIR = BASE_DIR / "Exports"

MESH_NAME = "SM_DeskPartition"

# Material slots (slot names in Unreal come from these names).
MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Panel_Plastic", (0.10, 0.29, 0.235, 1.0), 0.0, 0.6),
    ("Glass", (0.42, 0.47, 0.45, 0.35), 0.0, 0.35),
]
I_PANEL, I_GLASS = 0, 1
GLASS_OPACITY = 0.35

# Layout (Unreal cm).
X0, X1 = -100.5, 99.5
HALF_T = 3.0              # frame thickness 6 cm (y -3 .. 3)
FRAME_W = 4.0             # frame stile / rail width
BOTTOM_RAIL = 5.0
PANEL_TOP = 87.0          # top of the solid lower panel
INFILL_HALF_T = 2.1       # recessed infill panel (y -2.1 .. 2.1)
BEAD_INSET = 2.0          # decorative bead, inset from the frame
BEAD_W, BEAD_PROUD = 0.9, 0.35
GLASS_HALF_T = 0.8        # 1.6 cm glass pane
GLASS_X0, GLASS_X1 = -100.0, 99.0
GLASS_Z0, GLASS_Z1 = 85.0, 150.2


def ue_box(x0, x1, y0, y1, z0, z1):
    """Unreal cm box extents -> Blender meter extents (x0, x1, y0, y1, z0, z1)."""
    return (x0 / 100.0, x1 / 100.0, -y1 / 100.0, -y0 / 100.0, z0 / 100.0, z1 / 100.0)


def part(bm, x0, x1, y0, y1, z0, z1, mat, bevel=0.0, seg=2):
    c = ue_box(x0, x1, y0, y1, z0, z1)
    p = rh.beveled_box(*c, bevel / 100.0, mat, seg) if bevel > 0 else rh.box(*c, mat)
    rh.merge_into(bm, p)


def build_partition():
    bm = bmesh.new()
    t = HALF_T
    ix0, ix1 = X0 + FRAME_W, X1 - FRAME_W            # inner edges of the stiles
    iz0, iz1 = BOTTOM_RAIL, PANEL_TOP - FRAME_W      # inner edges of the rails

    # Frame: two stiles and two rails, softened edges.
    part(bm, X0, ix0, -t, t, 0.0, PANEL_TOP, I_PANEL, 0.5, 2)
    part(bm, ix1, X1, -t, t, 0.0, PANEL_TOP, I_PANEL, 0.5, 2)
    part(bm, ix0 - 0.5, ix1 + 0.5, -t + 0.15, t - 0.15, 0.0, iz0, I_PANEL, 0.4, 2)
    part(bm, ix0 - 0.5, ix1 + 0.5, -t + 0.15, t - 0.15, iz1, PANEL_TOP, I_PANEL, 0.4, 2)

    # Recessed infill panel.
    part(bm, ix0 - 0.5, ix1 + 0.5, -INFILL_HALF_T, INFILL_HALF_T, iz0 - 0.5, iz1 + 0.5, I_PANEL)

    # Decorative bead rectangle on both faces of the infill.
    bx0, bx1 = ix0 + BEAD_INSET, ix1 - BEAD_INSET
    bz0, bz1 = iz0 + BEAD_INSET, iz1 - BEAD_INSET
    for ya, yb in ((INFILL_HALF_T - 0.1, INFILL_HALF_T + BEAD_PROUD),
                   (-INFILL_HALF_T - BEAD_PROUD, -INFILL_HALF_T + 0.1)):
        part(bm, bx0, bx1, ya, yb, bz0, bz0 + BEAD_W, I_PANEL, 0.15, 1)
        part(bm, bx0, bx1, ya, yb, bz1 - BEAD_W, bz1, I_PANEL, 0.15, 1)
        # Vertical strips overlap the horizontal ones slightly (no coincident corner vertices).
        part(bm, bx0, bx0 + BEAD_W, ya, yb, bz0 + 0.37, bz1 - 0.37, I_PANEL, 0.15, 1)
        part(bm, bx1 - BEAD_W, bx1, ya, yb, bz0 + 0.37, bz1 - 0.37, I_PANEL, 0.15, 1)

    # Glazing beads holding the glass on top of the frame.
    for sgn in (-1.0, 1.0):
        g0, g1 = sorted((sgn * GLASS_HALF_T, sgn * (GLASS_HALF_T + 1.0)))
        part(bm, X0 + 0.6, X1 - 0.6, g0, g1, PANEL_TOP - 0.5, PANEL_TOP + 1.6, I_PANEL, 0.3, 1)

    # Dirty glass pane.
    part(bm, GLASS_X0, GLASS_X1, -GLASS_HALF_T, GLASS_HALF_T, GLASS_Z0, GLASS_Z1, I_GLASS, 0.25, 1)
    return bm


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def preview_dir():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--preview" in argv:
        return Path(argv[argv.index("--preview") + 1])
    return None


def make_materials():
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    glass = mats[I_GLASS]
    bsdf = next(n for n in glass.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Alpha"].default_value = GLASS_OPACITY
    glass.surface_render_method = "BLENDED"
    return mats


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Desk_Partition_4X (cubicle divider).",
        Pivot="Floor level, center of the thickness; spans x -1.005 .. 0.995 m. Both faces identical.",
    )
    mats = make_materials()
    obj = rh.finish_object(build_partition(), MESH_NAME, mats, uv_scale=1.0, sharp_angle_deg=35.0)

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
