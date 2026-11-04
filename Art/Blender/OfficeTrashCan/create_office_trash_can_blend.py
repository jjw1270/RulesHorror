import bpy
import bmesh
import math
from pathlib import Path
from mathutils import Matrix, Vector

# RulesHorror UE-ready office pedal trash can generator.
# Replaces /Game/Assets/BrutalistLevelKit/.../SM_Trash_Can (purchased asset).
# Same footprint and pivot as the original so placed actors keep their transform:
#   pivot = bottom center of the body, +X = front (pedal side), Z up, meters.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "OfficeTrashCan.blend"
EXPORT_DIR = BASE_DIR / "Exports"
OUTPUT_FBX = EXPORT_DIR / "SM_OfficeTrashCan.fbx"

MESH_NAME = "SM_OfficeTrashCan"

# Material slots (slot names in Unreal come from these names).
MAT_BODY = "Body_Paint"
MAT_METAL = "Trim_Metal"
MAT_PLASTIC = "Lid_Plastic"
MATERIALS = [
    # name, base color (linear), metallic, roughness
    (MAT_BODY, (0.012, 0.016, 0.03, 1.0), 0.6, 0.45),
    (MAT_METAL, (0.55, 0.56, 0.58, 1.0), 1.0, 0.3),
    (MAT_PLASTIC, (0.01, 0.014, 0.035, 1.0), 0.0, 0.55),
]
I_BODY, I_METAL, I_PLASTIC = 0, 1, 2

# Dimensions (meters).
BODY_X = 0.440  # depth (front-back)
BODY_Y = 0.500  # width
CORNER_R = 0.075  # vertical corner radius
BASE_H = 0.014
BODY_TOP = 0.835
RIM_TOP = 0.855
LID_TOP = 0.902


# -----------------------------------------------------------------------------
# Scene / materials
# -----------------------------------------------------------------------------


def ensure_dirs():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)


def clear_scene():
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode="OBJECT")
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for collection in (bpy.data.meshes, bpy.data.materials, bpy.data.images,
                       bpy.data.cameras, bpy.data.lights, bpy.data.curves):
        for datablock in list(collection):
            if datablock.users == 0:
                collection.remove(datablock)


def set_scene():
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene["AssetPurpose"] = "Self-made replacement for BrutalistLevelKit SM_Trash_Can (office pedal bin)."
    scene["Pivot"] = "Bottom center of the body. +X = front (pedal side), Z up."
    scene["ExportFBX"] = str(OUTPUT_FBX)


def make_materials():
    mats = []
    for name, color, metallic, roughness in MATERIALS:
        mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
        bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        mat.diffuse_color = color
        mats.append(mat)
    return mats


# -----------------------------------------------------------------------------
# Geometry helpers (each part is built in its own bmesh, then merged)
# -----------------------------------------------------------------------------


def rounded_box(sx, sy, z0, z1, corner_r, edge_r, mat_index, center=(0.0, 0.0), edge_segments=2):
    """Box with rounded vertical corners and softened top/bottom edges."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    sz = z1 - z0
    for v in bm.verts:
        v.co = Vector((v.co.x * sx + center[0], v.co.y * sy + center[1], v.co.z * sz + (z0 + z1) * 0.5))

    eps = 1e-6
    vertical = [e for e in bm.edges
                if abs(e.verts[0].co.x - e.verts[1].co.x) < eps and abs(e.verts[0].co.y - e.verts[1].co.y) < eps]
    if corner_r > 0:
        bmesh.ops.bevel(bm, geom=vertical, offset=corner_r, segments=8, profile=0.5,
                        affect="EDGES", clamp_overlap=True)
    if edge_r > 0:
        caps = [f for f in bm.faces if abs(f.normal.z) > 0.99]
        cap_edges = list({e for f in caps for e in f.edges})
        bmesh.ops.bevel(bm, geom=cap_edges, offset=edge_r, segments=edge_segments, profile=0.5,
                        affect="EDGES", clamp_overlap=True)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def cylinder_y(radius, length, center, mat_index, segments=16):
    bm = bmesh.new()
    rot = Matrix.Translation(Vector(center)) @ Matrix.Rotation(math.radians(90.0), 4, "X")
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius, depth=length, matrix=rot)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def merge_into(dst, src):
    vmap = {}
    for v in src.verts:
        vmap[v] = dst.verts.new(v.co)
    for f in src.faces:
        nf = dst.faces.new([vmap[v] for v in f.verts])
        nf.material_index = f.material_index
    src.free()


def box_project_uvs(bm, scale=1.0):
    uv_layer = bm.loops.layers.uv.verify()
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for loop in f.loops:
            co = loop.vert.co
            if ax == 0:
                uv = (co.y, co.z)
            elif ax == 1:
                uv = (co.x, co.z)
            else:
                uv = (co.x, co.y)
            loop[uv_layer].uv = (uv[0] * scale, uv[1] * scale)


def set_smoothing(bm, sharp_angle_deg=35.0):
    limit = math.radians(sharp_angle_deg)
    for f in bm.faces:
        f.smooth = True
    for e in bm.edges:
        if len(e.link_faces) != 2:
            e.smooth = False
            continue
        e.smooth = e.calc_face_angle(0.0) <= limit


# -----------------------------------------------------------------------------
# Trash can
# -----------------------------------------------------------------------------


def create_mesh(mats):
    bm = bmesh.new()

    # Metal foot band.
    merge_into(bm, rounded_box(BODY_X + 0.006, BODY_Y + 0.006, 0.0, BASE_H,
                               CORNER_R + 0.003, 0.003, I_METAL))
    # Painted body.
    merge_into(bm, rounded_box(BODY_X, BODY_Y, BASE_H, BODY_TOP,
                               CORNER_R, 0.002, I_BODY))
    # Metal top rim.
    merge_into(bm, rounded_box(BODY_X + 0.008, BODY_Y + 0.008, BODY_TOP, RIM_TOP,
                               CORNER_R + 0.004, 0.004, I_METAL))
    # Lid: slightly domed by a large top-edge bevel.
    merge_into(bm, rounded_box(BODY_X + 0.004, BODY_Y + 0.004, RIM_TOP, LID_TOP,
                               CORNER_R + 0.002, 0.018, I_PLASTIC, edge_segments=4))
    # Rear hinge.
    merge_into(bm, cylinder_y(0.011, 0.16, (-BODY_X * 0.5 - 0.006, 0.0, RIM_TOP + 0.004), I_METAL))
    # Front pedal (plastic) on a metal bracket.
    merge_into(bm, rounded_box(0.040, 0.050, 0.004, 0.026, 0.008, 0.002, I_METAL,
                               center=(BODY_X * 0.5 + 0.012, 0.0)))
    merge_into(bm, rounded_box(0.050, 0.170, 0.010, 0.034, 0.012, 0.004, I_PLASTIC,
                               center=(BODY_X * 0.5 + 0.022, 0.0)))
    # Small metal label plate on the front.
    merge_into(bm, rounded_box(0.003, 0.090, BODY_TOP - 0.075, BODY_TOP - 0.040, 0.0, 0.0, I_METAL,
                               center=(BODY_X * 0.5 + 0.0015, 0.0)))

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    set_smoothing(bm)
    box_project_uvs(bm)

    mesh = bpy.data.meshes.new(MESH_NAME + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for mat in mats:
        mesh.materials.append(mat)

    obj = bpy.data.objects.new(MESH_NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


# -----------------------------------------------------------------------------
# Export
# -----------------------------------------------------------------------------


def export_fbx(mesh_obj):
    bpy.ops.object.select_all(action="DESELECT")
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = mesh_obj
    bpy.ops.export_scene.fbx(
        filepath=str(OUTPUT_FBX),
        use_selection=True,
        object_types={"MESH"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        path_mode="AUTO",
        axis_forward="-Z",
        axis_up="Y",
        global_scale=1.0,
        apply_unit_scale=True,
        use_space_transform=True,
        bake_anim=False,
    )


def main():
    ensure_dirs()
    clear_scene()
    set_scene()
    mats = make_materials()
    mesh_obj = create_mesh(mats)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    export_fbx(mesh_obj)
    d = mesh_obj.dimensions
    print(f"Dimensions (m): {d.x:.3f} x {d.y:.3f} x {d.z:.3f}, tris={sum(len(p.vertices) - 2 for p in mesh_obj.data.polygons)}")
    print(f"Saved blend: {OUTPUT_BLEND}")
    print(f"Exported FBX: {OUTPUT_FBX}")


if __name__ == "__main__":
    main()
