import math
import sys
from pathlib import Path

import bmesh
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready fluorescent ceiling fixture.
# Replaces /Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/Lights/SM_Ceiling_Lamp (purchased asset),
# used by /Game/Objects/Ceiling_Light_BP. Same bounds and pivot as the original:
#   pivot = ceiling mount point (top of the fixture), the fixture hangs below it, long axis = X.
#   The opal diffuser is on the Unreal +Y side (Blender -Y), the mounting rail behind it on Unreal -Y.
# Model coordinates below are written as Unreal (x, y, z) in cm and converted with ue().

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "CeilingLamp.blend"
OUTPUT_FBX = BASE_DIR / "Exports" / "SM_CeilingLamp.fbx"
MESH_NAME = "SM_CeilingLamp"

MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Tube_Emissive", (0.85, 0.86, 0.85, 1.0), 0.0, 0.35),
    ("Metal_Housing", (0.45, 0.50, 0.55, 1.0), 0.5, 0.45),
    ("Metal_Rail", (0.20, 0.21, 0.22, 1.0), 0.7, 0.4),
]
I_DIFF, I_HOUS, I_RAIL = 0, 1, 2

# Extents (Unreal cm).
X0, X1 = -88.9, 85.9          # overall
DX0, DX1 = -85.0, 82.0        # diffuser / housing body
CAP = 0.35                    # end cap overhang around the diffuser
DIFF_Y = (-6.5, 9.6 - CAP)    # opal diffuser (front, Unreal +Y side)
DIFF_Z = (-21.7 + CAP, -5.6)
HOUS_Y = (-16.0, -5.5)        # metal back housing with a perforated top
HOUS_Z = (-17.5, -4.6)
RAIL_Y, RAIL_Z, RAIL_R = -20.0, -3.2, 1.7


def ue(x, y, z):
    """Unreal cm -> Blender m."""
    return (x * 0.01, -y * 0.01, z * 0.01)


def ue_box(x0, x1, y0, y1, z0, z1, mat, bevel=0.0, segments=2):
    bm = rh.box(x0 * 0.01, x1 * 0.01, -y1 * 0.01, -y0 * 0.01, z0 * 0.01, z1 * 0.01, mat)
    rh.bevel_edges(bm, bm.edges, bevel * 0.01, segments)
    for f in bm.faces:
        f.material_index = mat
    return bm


def rounded_profile(y0, y1, z0, z1, r_bottom, r_top, k=6):
    """(y, z) loop of a rectangle with separate bottom/top corner radii (Unreal cm)."""
    pts = []
    corners = [(y1 - r_top, z1 - r_top, r_top, 0.0), (y0 + r_top, z1 - r_top, r_top, 90.0),
               (y0 + r_bottom, z0 + r_bottom, r_bottom, 180.0), (y1 - r_bottom, z0 + r_bottom, r_bottom, 270.0)]
    for cy, cz, r, a0 in corners:
        for i in range(k + 1):
            a = math.radians(a0 + 90.0 * i / k)
            pts.append((cy + r * math.cos(a), cz + r * math.sin(a)))
    return pts


def extrude_x(profile, x0, x1, mat, x_steps=1):
    """Extrude a closed (y, z) profile along X (Unreal cm). Caps are triangle fans."""
    bm = bmesh.new()
    n = len(profile)
    rings = []
    for s in range(x_steps + 1):
        x = x0 + (x1 - x0) * s / x_steps
        rings.append([bm.verts.new(ue(x, y, z)) for y, z in profile])
    for a, b in zip(rings[:-1], rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((a[i], a[j], b[j], b[i]))
    cy = sum(p[0] for p in profile) / n
    cz = sum(p[1] for p in profile) / n
    for ring, x in ((rings[0], x0), (rings[-1], x1)):
        c = bm.verts.new(ue(x, cy, cz))
        for i in range(n):
            bm.faces.new((ring[i], ring[(i + 1) % n], c))
    for f in bm.faces:
        f.material_index = mat
    return bm


def rail_cylinder(x0, x1, y, z, r, mat, segments=12):
    length = x1 - x0
    return rh.cylinder(r * 0.01, length * 0.01, ue((x0 + x1) * 0.5, y, z), axis="X",
                       mat_index=mat, segments=segments)


def end_assembly(bm, side):
    """Clamp on the rail, ceiling tab, bracket plate and end cap. side=-1 (X0 end) or +1 (X1 end)."""
    xo, xi = (X0, DX0) if side < 0 else (X1, DX1)
    xa, xb = min(xo, xo - side * 5.6), max(xo, xo - side * 5.6)
    # Rail clamp: block with a collar around the rail, plus the ceiling tab.
    rh.merge_into(bm, ue_box(xa, xb, -24.5, -15.6, -7.0, 0.2, I_RAIL, bevel=0.6))
    ka, kb = (xb, xb + 1.6) if side < 0 else (xa - 1.6, xa)
    rh.merge_into(bm, rail_cylinder(ka, kb, RAIL_Y, RAIL_Z, RAIL_R + 0.9, I_RAIL, segments=12))
    rh.merge_into(bm, ue_box(xa + 1.0, xb - 1.0, -23.0, -17.0, -0.8, 1.5, I_RAIL, bevel=0.3))
    # Bracket plate on the outside of the end cap, joined to the clamp.
    pa, pb = (xa, xa + 1.1) if side < 0 else (xb - 1.1, xb)
    rh.merge_into(bm, ue_box(pa, pb, -19.0, -3.0, -15.0, -2.0, I_RAIL, bevel=0.3))
    # End cap over the diffuser and housing ends.
    cap = rounded_profile(DIFF_Y[0] - CAP, DIFF_Y[1] + CAP, DIFF_Z[0] - CAP, DIFF_Z[1] + CAP, 5.5 + CAP, 2.5 + CAP)
    ca, cb = (xa + 1.0, xi + 0.8) if side < 0 else (xi - 0.8, xb - 1.0)
    rh.merge_into(bm, extrude_x(cap, ca, cb, I_HOUS))
    rh.merge_into(bm, ue_box(ca, cb, HOUS_Y[0] - CAP, HOUS_Y[1], HOUS_Z[0] - CAP, HOUS_Z[1] + CAP, I_HOUS,
                             bevel=0.3))


def create_mesh(mats):
    bm = bmesh.new()

    # Opal diffuser (emissive), in front of the housing.
    diff = rounded_profile(DIFF_Y[0], DIFF_Y[1], DIFF_Z[0], DIFF_Z[1], 5.5, 2.5)
    rh.merge_into(bm, extrude_x(diff, DX0, DX1, I_DIFF))

    # Metal back housing with a row of dark vent slots on top.
    rh.merge_into(bm, ue_box(DX0, DX1, HOUS_Y[0], HOUS_Y[1], HOUS_Z[0], HOUS_Z[1], I_HOUS, bevel=0.6))
    n_slots = 30
    span = (DX1 - DX0) - 8.0
    for i in range(n_slots):
        cx = DX0 + 4.0 + span * (i + 0.5) / n_slots
        rh.merge_into(bm, ue_box(cx - 1.9, cx + 1.9, -13.8, -8.0, HOUS_Z[1] - 0.2, HOUS_Z[1] + 0.1, I_RAIL,
                                 bevel=0.08, segments=1))

    # Mounting rail behind and above the housing.
    rh.merge_into(bm, rail_cylinder(X0 + 1.0, X1 - 1.0, RAIL_Y, RAIL_Z, RAIL_R, I_RAIL))

    end_assembly(bm, -1)
    end_assembly(bm, 1)

    return rh.finish_object(bm, MESH_NAME, mats, sharp_angle_deg=40.0)


def main():
    rh.clear_scene()
    rh.set_scene(AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Ceiling_Lamp (fluorescent fixture).",
                 Pivot="Ceiling mount point at the top; fixture hangs below. Long axis X, diffuser on Unreal +Y.",
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
