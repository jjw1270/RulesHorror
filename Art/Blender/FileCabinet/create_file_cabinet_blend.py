import sys
from pathlib import Path

import math

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready vertical metal filing cabinet (3 drawers, like the original).
# Replaces BrutalistLevelKit SM_File_Cabinet1 (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = floor level, centered in X, at the back face (Unreal y = 0.6 .. 79.1).
#   Drawer fronts face Unreal +Y (Blender -Y). Z up, meters.
#
# Layout numbers are written in Unreal centimeters and converted with ue_box(); Blender y = -Unreal y.
#
# Optional preview render (not saved into the .blend):
#   blender --background --factory-startup --python create_file_cabinet_blend.py -- --preview <dir>

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "FileCabinet.blend"
EXPORT_DIR = BASE_DIR / "Exports"

MESH_NAME = "SM_FileCabinet"

# Material slots (slot names in Unreal come from these names).
MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Body_Paint", (0.055, 0.15, 0.13, 1.0), 0.3, 0.5),
    ("Metal_Dark", (0.05, 0.055, 0.06, 1.0), 1.0, 0.4),
    ("Label_Paper", (0.40, 0.38, 0.32, 1.0), 0.0, 0.85),
]
I_BODY, I_METAL, I_PAPER = 0, 1, 2

# Layout (Unreal cm).
X_HALF = 27.1                 # top cap half width
BODY_X_HALF = 26.8
BACK_Y = 0.6
HANDLE_FRONT_Y = 79.1         # front of the hanging bail pulls (= original bounds)
BAIL_PIVOT, BAIL_LEN, BAIL_R, BAIL_ANGLE = 0.9, 5.0, 0.65, 40.0   # cm, cm, cm, degrees below horizontal
DRAWER_FRONT_Y = HANDLE_FRONT_Y - (BAIL_PIVOT + BAIL_LEN * math.cos(math.radians(BAIL_ANGLE)) + BAIL_R + 0.2)
BODY_FRONT_Y = DRAWER_FRONT_Y - 1.3   # carcass front plane
CAP_FRONT_Y = DRAWER_FRONT_Y + 0.3
TOP_Z = 99.9
CAP_T = 1.4
DRAWER_X_HALF = 25.5          # leaves a ~1.3 cm stile of the carcass visible
DRAWER_Z0, DRAWER_Z1 = 3.8, 96.6
DRAWERS = 3
GAP = 0.5


def ue_box(x0, x1, y0, y1, z0, z1):
    """Unreal cm box extents -> Blender meter extents (x0, x1, y0, y1, z0, z1)."""
    return (x0 / 100.0, x1 / 100.0, -y1 / 100.0, -y0 / 100.0, z0 / 100.0, z1 / 100.0)


def part(bm, x0, x1, y0, y1, z0, z1, mat, bevel=0.0, seg=2):
    c = ue_box(x0, x1, y0, y1, z0, z1)
    p = rh.beveled_box(*c, bevel / 100.0, mat, seg) if bevel > 0 else rh.box(*c, mat)
    rh.merge_into(bm, p)


def label_holder(bm, cz, w=19.0, h=5.4):
    """Dark metal card frame with a paper card behind the lips."""
    fy = DRAWER_FRONT_Y
    part(bm, -w / 2, w / 2, fy - 0.1, fy + 0.25, cz - h / 2, cz + h / 2, I_METAL, 0.1, 1)       # back plate
    part(bm, -w / 2 + 0.7, w / 2 - 0.7, fy + 0.2, fy + 0.35, cz - h / 2 + 0.7, cz + h / 2 - 0.7, I_PAPER)
    lip = 0.7
    part(bm, -w / 2, w / 2, fy + 0.2, fy + 0.65, cz + h / 2 - lip, cz + h / 2, I_METAL, 0.12, 1)  # top lip
    part(bm, -w / 2, w / 2, fy + 0.2, fy + 0.65, cz - h / 2, cz - h / 2 + lip, I_METAL, 0.12, 1)  # bottom lip
    for sx in (-1.0, 1.0):
        x_out = sx * w / 2
        x_in = sx * (w / 2 - lip)
        part(bm, min(x_out, x_in), max(x_out, x_in), fy + 0.2, fy + 0.65,
             cz - h / 2 + lip * 0.4, cz + h / 2 - lip * 0.4, I_METAL, 0.12, 1)


def pull_handle(bm, cz, w=20.0):
    """Drop-bail pull: two mounting brackets and a wire loop hanging forward/down at BAIL_ANGLE."""
    fy = DRAWER_FRONT_Y
    pz = cz + 1.5                      # pivot height
    py = fy + BAIL_PIVOT               # pivot distance in front of the drawer
    for sx in (-1.0, 1.0):
        x = sx * (w / 2 + 0.2)
        part(bm, x - 0.8, x + 0.8, fy - 0.1, py + 0.35, pz - 1.4, pz + 0.8, I_METAL, 0.2, 1)   # bracket
    # Bail: a flat stamped ring, built horizontally (pointing to Unreal +Y), then rotated down
    # around the pivot. Strips are 1.4 cm wide so the loop reads as a frame from the front.
    bail = bmesh.new()
    strip, th = 1.4, 0.5
    y_far = py + BAIL_LEN + BAIL_R
    for sx in (-1.0, 1.0):
        x = sx * (w / 2 - strip / 2)
        rh.merge_into(bail, rh.beveled_box(*ue_box(x - strip / 2, x + strip / 2, py - 0.5, y_far,
                                                   pz - th / 2, pz + th / 2), 0.0015, I_METAL, 1))
    # Cross bars overlap into the arms (no coincident vertices).
    xb = w / 2 - strip * 0.6
    rh.merge_into(bail, rh.beveled_box(*ue_box(-xb, xb, y_far - 1.6, y_far - 0.03,
                                               pz - th / 2 + 0.03, pz + th / 2 - 0.03), 0.0015, I_METAL, 1))
    rh.merge_into(bail, rh.beveled_box(*ue_box(-xb, xb, py - 0.4, py + 0.7,
                                               pz - th / 2 + 0.03, pz + th / 2 - 0.03), 0.0015, I_METAL, 1))
    pivot = Vector((0.0, -py / 100.0, pz / 100.0))
    rh.transform(bail, Matrix.Translation(pivot) @ Matrix.Rotation(math.radians(BAIL_ANGLE), 4, "X")
                 @ Matrix.Translation(-pivot))
    rh.merge_into(bm, bail)


def build_cabinet():
    bm = bmesh.new()

    # Carcass and top cap.
    part(bm, -BODY_X_HALF, BODY_X_HALF, BACK_Y, BODY_FRONT_Y, 0.0, TOP_Z - CAP_T + 0.2, I_BODY, 0.6, 2)
    part(bm, -X_HALF, X_HALF, BACK_Y, CAP_FRONT_Y, TOP_Z - CAP_T, TOP_Z, I_BODY, 0.5, 2)
    # Recessed kick at the bottom front.
    part(bm, -BODY_X_HALF + 1.0, BODY_X_HALF - 1.0, BODY_FRONT_Y - 1.0, BODY_FRONT_Y + 0.6, 0.0, DRAWER_Z0 - GAP,
         I_METAL, 0.2, 1)

    h = (DRAWER_Z1 - DRAWER_Z0 - GAP * (DRAWERS - 1)) / DRAWERS
    for i in range(DRAWERS):
        z1 = DRAWER_Z1 - i * (h + GAP)
        z0 = z1 - h
        # Drawer front: sheet-metal panel with rolled edges.
        part(bm, -DRAWER_X_HALF, DRAWER_X_HALF, BODY_FRONT_Y - 0.5, DRAWER_FRONT_Y, z0, z1, I_BODY, 0.45, 2)
        label_holder(bm, z1 - 5.2)
        pull_handle(bm, z0 + 6.0)
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
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_File_Cabinet1 (vertical filing cabinet).",
        Pivot="Floor level, centered in X, back face at Blender y=-0.006. Drawers face Blender -Y (Unreal +Y).",
    )
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    obj = rh.finish_object(build_cabinet(), MESH_NAME, mats, uv_scale=1.0, sharp_angle_deg=35.0)

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
