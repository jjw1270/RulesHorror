"""Shared helpers for RulesHorror prop generator scripts (Blender 5.1).

Usage from a generator script in Art/Blender/<Prop>/:

    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
    import rh_prop_utils as rh

Axis convention (verified by import test):
    Blender +X -> Unreal +X, Blender +Y -> Unreal -Y, Blender +Z -> Unreal +Z.
    So a face that must point to Unreal +Y is modeled facing Blender -Y.
Units: meters in Blender, exported with unit scale applied (1 m = 100 cm in Unreal).
"""

import math
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector


# -----------------------------------------------------------------------------
# Scene / materials
# -----------------------------------------------------------------------------


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


def set_scene(**props):
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    for key, value in props.items():
        scene[key] = value


def make_material(name, color, metallic=0.0, roughness=0.5, image=None):
    """Principled material. color is linear RGBA. image: optional bpy image for Base Color."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    mat.diffuse_color = color
    if image is not None:
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = image
        tex.location = (-400, 200)
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
        nodes.active = tex
    return mat


def save_image(image, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    return path


# -----------------------------------------------------------------------------
# Geometry (each part is built in its own bmesh, then merged with merge_into)
# -----------------------------------------------------------------------------


def box(x0, x1, y0, y1, z0, z1, mat_index=0):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    sx, sy, sz = x1 - x0, y1 - y0, z1 - z0
    cx, cy, cz = (x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5
    for v in bm.verts:
        v.co = Vector((v.co.x * sx + cx, v.co.y * sy + cy, v.co.z * sz + cz))
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def bevel_edges(bm, edges, offset, segments=2):
    if offset > 0 and edges:
        bmesh.ops.bevel(bm, geom=list(edges), offset=offset, segments=segments, profile=0.5,
                        affect="EDGES", clamp_overlap=True)
    return bm


def edges_along(bm, axis, eps=1e-6):
    """Edges parallel to axis (0=X, 1=Y, 2=Z)."""
    others = [i for i in range(3) if i != axis]
    return [e for e in bm.edges
            if all(abs(e.verts[0].co[i] - e.verts[1].co[i]) < eps for i in others)]


def rounded_box(sx, sy, z0, z1, corner_r, edge_r, mat_index, center=(0.0, 0.0), edge_segments=2):
    """Box (size sx, sy, from z0 to z1) with rounded vertical corners and softened cap edges."""
    bm = box(center[0] - sx * 0.5, center[0] + sx * 0.5, center[1] - sy * 0.5, center[1] + sy * 0.5,
             z0, z1, mat_index)
    if corner_r > 0:
        bmesh.ops.bevel(bm, geom=edges_along(bm, 2), offset=corner_r, segments=8, profile=0.5,
                        affect="EDGES", clamp_overlap=True)
    if edge_r > 0:
        caps = [f for f in bm.faces if abs(f.normal.z) > 0.99]
        bmesh.ops.bevel(bm, geom=list({e for f in caps for e in f.edges}), offset=edge_r,
                        segments=edge_segments, profile=0.5, affect="EDGES", clamp_overlap=True)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def beveled_box(x0, x1, y0, y1, z0, z1, bevel, mat_index=0, segments=2):
    """Axis-aligned box with all edges beveled (good default for hard-surface parts)."""
    bm = box(x0, x1, y0, y1, z0, z1, mat_index)
    bevel_edges(bm, bm.edges, bevel, segments)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def cylinder(radius, depth, center, axis="Z", mat_index=0, segments=16, radius2=None):
    """Cylinder (or cone when radius2 is given) centered at center, along axis X/Y/Z."""
    bm = bmesh.new()
    rot = {"Z": Matrix.Identity(4),
           "X": Matrix.Rotation(math.radians(90.0), 4, "Y"),
           "Y": Matrix.Rotation(math.radians(90.0), 4, "X")}[axis]
    mat = Matrix.Translation(Vector(center)) @ rot
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius if radius2 is None else radius2,
                          depth=depth, matrix=mat)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def transform(bm, matrix):
    bmesh.ops.transform(bm, matrix=matrix, verts=bm.verts)
    return bm


def merge_into(dst, src):
    """Append src geometry (keeping material indices) into dst and free src."""
    vmap = {v: dst.verts.new(v.co) for v in src.verts}
    for f in src.faces:
        nf = dst.faces.new([vmap[v] for v in f.verts])
        nf.material_index = f.material_index
        nf.smooth = f.smooth
    src.free()
    return dst


def box_project_uvs(bm, scale=1.0):
    """Box projection: 1 UV unit = 1/scale meters. Good enough for tiling/procedural materials."""
    uv_layer = bm.loops.layers.uv.verify()
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for loop in f.loops:
            co = loop.vert.co
            uv = (co.y, co.z) if ax == 0 else (co.x, co.z) if ax == 1 else (co.x, co.y)
            loop[uv_layer].uv = (uv[0] * scale, uv[1] * scale)


def set_smoothing(bm, sharp_angle_deg=35.0):
    limit = math.radians(sharp_angle_deg)
    for f in bm.faces:
        f.smooth = True
    for e in bm.edges:
        e.smooth = len(e.link_faces) == 2 and e.calc_face_angle(0.0) <= limit


def finish_object(bm, name, materials, uv_scale=1.0, sharp_angle_deg=35.0, keep_uvs=False):
    """Recalc normals, smoothing, UVs, then create the mesh object in the scene."""
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    set_smoothing(bm, sharp_angle_deg)
    if not keep_uvs:
        box_project_uvs(bm, uv_scale)
    mesh = bpy.data.meshes.new(name + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for mat in materials:
        mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def report(obj):
    """Print Unreal-space bounds (cm) so they can be compared with the original asset."""
    xs = [v.co.x for v in obj.data.vertices]
    ys = [-v.co.y for v in obj.data.vertices]  # Unreal Y = -Blender Y
    zs = [v.co.z for v in obj.data.vertices]
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f"{obj.name}: UE bounds cm min=({min(xs)*100:.1f}, {min(ys)*100:.1f}, {min(zs)*100:.1f}) "
          f"max=({max(xs)*100:.1f}, {max(ys)*100:.1f}, {max(zs)*100:.1f}) tris={tris} "
          f"slots={[m.name for m in obj.data.materials]}")


# -----------------------------------------------------------------------------
# Export / preview
# -----------------------------------------------------------------------------


def export_fbx(obj, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"}, use_mesh_modifiers=True,
        mesh_smooth_type="FACE", path_mode="AUTO", axis_forward="-Z", axis_up="Y",
        global_scale=1.0, apply_unit_scale=True, use_space_transform=True, bake_anim=False)
    return path


def preview_render(objs, out_png, resolution=512, azimuth_deg=65.0, elevation_deg=20.0):
    """Render a preview from the same side as Unreal's asset thumbnail:
    Unreal +Y face on the left, Unreal +X face on the right, looking slightly down.
    Temporary camera/light are removed afterwards so the saved .blend stays clean."""
    if not isinstance(objs, (list, tuple)):
        objs = [objs]
    scene = bpy.context.scene
    pts = [o.matrix_world @ v.co for o in objs for v in o.data.vertices]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    center = (lo + hi) * 0.5
    radius = max((hi - lo).length * 0.5, 0.01)

    az, el = math.radians(azimuth_deg), math.radians(elevation_deg)
    direction = Vector((math.cos(az) * math.cos(el), -math.sin(az) * math.cos(el), math.sin(el)))
    cam_data = bpy.data.cameras.new("_PreviewCam")
    cam_data.lens = 50.0
    cam = bpy.data.objects.new("_PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    fov = 2.0 * math.atan(cam_data.sensor_width / (2.0 * cam_data.lens))
    dist = radius / math.sin(fov * 0.5) * 1.05
    cam.location = center + direction * dist
    cam.rotation_euler = (-direction).to_track_quat("-Z", "Y").to_euler()
    cam_data.clip_start = dist * 0.01
    cam_data.clip_end = dist * 10.0

    prev_cam = scene.camera
    scene.camera = cam
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    scene.display.shading.show_cavity = True
    scene.display.shading.show_shadows = True
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    out_png = Path(out_png)
    out_png.parent.mkdir(parents=True, exist_ok=True)
    scene.render.filepath = str(out_png)
    bpy.ops.render.render(write_still=True)

    scene.camera = prev_cam
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.cameras.remove(cam_data)
    return out_png
