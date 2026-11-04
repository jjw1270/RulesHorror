import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready coffee mug generator.
# Replaces /Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_Food_Leftover_EO1_N1 (purchased asset).
# Same bounds and pivot as the original so placed actors keep their transform:
#   pivot = center of the bounding box footprint (body + handle), Z = 0 at the bottom.
#   The body axis sits at Unreal (0, -1.5). Like the original, the handle does not point straight
#   along +Y: it points to Unreal +Y turned 28 deg toward +X (deduced from the original's bounds
#   and its saved thumbnail camera, which shows the handle in full profile).
#   Unreal bounds: X -4.5..4.5, Y -6.0..6.0, Z 0..9.7 cm.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "CoffeeMug.blend"
EXPORT_DIR = BASE_DIR / "Exports"
MESH_NAME = "SM_CoffeeMug"

# Material slots (slot names in Unreal come from these names). Linear colors.
MAT_GLAZE = "Ceramic_Glaze"
MAT_RIM = "Ceramic_Rim"
MAT_COFFEE = "Coffee_Leftover"
MATERIALS = [
    # name, base color (linear), metallic, roughness
    (MAT_GLAZE, (0.30, 0.012, 0.018, 1.0), 0.0, 0.22),
    (MAT_RIM, (0.80, 0.75, 0.68, 1.0), 0.0, 0.35),
    (MAT_COFFEE, (0.018, 0.007, 0.003, 1.0), 0.0, 0.12),
]
I_GLAZE, I_RIM, I_COFFEE = 0, 1, 2

SEGMENTS = 32            # around the body (multiple of 4 keeps the bounds exact)
BODY_CY = 0.015          # Blender Y of the body axis (= Unreal Y -1.5 cm)
R_OUT = 0.045            # outer radius
R_IN = 0.041             # inner radius (4 mm wall)
RIM_Z = 0.0944           # center of the rolled rim bead
RIM_R = 0.0026           # bead radius (top of rim = 9.70 cm)
RIM_BAND_Z = 0.0928      # white rim glaze starts here (outside and inside)
LIQUID_Z = 0.019         # leftover coffee surface
STAIN_Z = (0.0545, 0.0560)  # dried coffee ring on the inner wall
HANDLE_ANGLE = 28.0      # deg, handle direction = Unreal +Y turned toward Unreal +X


def lathe_profile():
    """(r, z, material of the segment to the next point), from bottom center around to the liquid center."""
    p = []
    p += [(0.0, 0.0025, I_GLAZE),          # recessed bottom center
          (0.0340, 0.0025, I_GLAZE),
          (0.0355, 0.0, I_GLAZE),           # foot ring
          (0.0425, 0.0, I_GLAZE),
          (0.0444, 0.0010, I_GLAZE),
          (R_OUT, 0.0035, I_GLAZE),
          (R_OUT, 0.0300, I_GLAZE),
          (R_OUT, 0.0650, I_GLAZE),
          (R_OUT, RIM_BAND_Z, I_RIM)]
    # Rolled rim bead: from the outside wall over the top to the inside wall.
    rc = (R_IN + R_OUT) * 0.5
    steps = 8
    for i in range(steps + 1):
        a = math.radians(-20.0 + (200.0 + 20.0) * i / steps)  # -20 .. 200 deg
        r = rc + math.cos(a) * (R_OUT - rc + 0.0003)
        z = RIM_Z + math.sin(a) * RIM_R
        p.append((r, z, I_RIM))
    p += [(R_IN, RIM_BAND_Z, I_GLAZE),
          (R_IN, 0.0700, I_GLAZE),
          (R_IN, STAIN_Z[1], I_COFFEE),
          (R_IN, STAIN_Z[0], I_GLAZE),
          (R_IN, 0.0300, I_GLAZE),
          (R_IN, LIQUID_Z + 0.0012, I_COFFEE),   # thin stain just above the liquid
          (R_IN - 0.0004, LIQUID_Z, I_COFFEE),   # meniscus
          (0.0250, LIQUID_Z, I_COFFEE),
          (0.0, LIQUID_Z, I_COFFEE)]
    return p


def lathe(profile, center, segments):
    bm = bmesh.new()
    rings = []
    for r, z, _ in profile:
        if r < 1e-9:
            rings.append([bm.verts.new((center[0], center[1], z))])
        else:
            ring = []
            for k in range(segments):
                a = 2.0 * math.pi * k / segments
                ring.append(bm.verts.new((center[0] + r * math.cos(a), center[1] + r * math.sin(a), z)))
            rings.append(ring)
    for i in range(len(profile) - 1):
        a, b, mat = rings[i], rings[i + 1], profile[i][2]
        for k in range(segments):
            k2 = (k + 1) % segments
            if len(a) == 1:
                f = bm.faces.new((a[0], b[k2], b[k]))
            elif len(b) == 1:
                f = bm.faces.new((a[k], a[k2], b[0]))
            else:
                f = bm.faces.new((a[k], a[k2], b[k2], b[k]))
            f.material_index = mat
    return bm


def catmull_rom(points, samples_per_seg=4):
    pts = [points[0]] + list(points) + [points[-1]]
    out = []
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = (Vector(pts[j]) for j in (i - 1, i, i + 1, i + 2))
        for s in range(samples_per_seg):
            t = s / samples_per_seg
            t2, t3 = t * t, t * t * t
            out.append(0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t2
                              + (-p0 + 3 * p1 - 3 * p2 + p3) * t3))
    out.append(Vector(points[-1]))
    return out


def handle():
    """Swept oval tube in the Blender Y-Z plane (x = 0), on the Blender -Y side of the body."""
    y_attach = BODY_CY - R_OUT + 0.0015       # ends sit inside the 4 mm wall
    half_w, half_t = 0.0060, 0.0036            # flat oval band: 12 mm wide, 7 mm thick
    ctrl = [(0.0, 0.0868), (0.0150, 0.0880), (0.0262, 0.0858), (0.0322, 0.0790),
            (0.0338, 0.0680), (0.0333, 0.0560), (0.0300, 0.0460), (0.0200, 0.0372),
            (0.0090, 0.0302), (0.0, 0.0270)]
    reach = 1.08  # scales the loop outward so the handle reaches the original's +Y bound
    path = [Vector((0.0, y_attach - u * reach, z)) for u, z in (tuple(c) for c in catmull_rom(ctrl, 3))]
    sides = 10
    bm = bmesh.new()
    rings = []
    for i, p in enumerate(path):
        t = (path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]).normalized()
        side = Vector((1.0, 0.0, 0.0))
        normal = t.cross(side).normalized()
        ring = []
        for k in range(sides):
            a = 2.0 * math.pi * k / sides
            ring.append(bm.verts.new(p + side * (math.cos(a) * half_w) + normal * (math.sin(a) * half_t)))
        rings.append(ring)
    for a, b in zip(rings[:-1], rings[1:]):
        for k in range(sides):
            k2 = (k + 1) % sides
            bm.faces.new((a[k], a[k2], b[k2], b[k]))
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    for f in bm.faces:
        f.material_index = I_GLAZE
    # Built pointing to Blender -Y; turn it about the body axis (Blender +Z rotation = Unreal +Y toward +X).
    rh.transform(bm, Matrix.Translation((0.0, BODY_CY, 0.0)) @ Matrix.Rotation(math.radians(HANDLE_ANGLE), 4, "Z")
                 @ Matrix.Translation((0.0, -BODY_CY, 0.0)))
    return bm


def create_mesh(mats):
    bm = lathe(lathe_profile(), (0.0, BODY_CY), SEGMENTS)
    rh.merge_into(bm, handle())
    return rh.finish_object(bm, MESH_NAME, mats, sharp_angle_deg=40.0)


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for Deko_MatrixDemo SM_Food_Leftover_EO1_N1 (red coffee mug).",
        Pivot="Bounding-box center at the bottom (Z=0). Body axis at Unreal (0,-1.5); handle toward Unreal +Y turned 28 deg to +X.",
    )
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    obj = create_mesh(mats)
    rh.report(obj)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, EXPORT_DIR / f"{MESH_NAME}.fbx")
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
