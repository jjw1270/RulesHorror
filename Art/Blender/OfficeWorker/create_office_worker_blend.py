"""Seated office worker for L_Office reflections, built with MPFB (MakeHuman for Blender).

Run headless with the MPFB extension installed (see README.md):
    blender --background --python create_office_worker_blend.py -- [--preview <dir>] [--no-save]
"""
import importlib
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

HERE = Path(__file__).resolve().parent
ART = HERE.parent
BLEND = HERE / "OfficeWorker.blend"
EXPORT_DIR = HERE / "Exports"
TEX_DIR = HERE / "Textures"


def dyn(pkg, key):
    """MPFB is a Blender extension; its absolute package name is only known at run time."""
    for name in list(sys.modules):
        if name.endswith(pkg):
            return getattr(importlib.import_module(name), key)
    raise RuntimeError(f"MPFB module {pkg} not found: install the MPFB extension (README.md)")


HumanService = dyn("mpfb.services.humanservice", "HumanService")
AssetService = dyn("mpfb.services.assetservice", "AssetService")

# -----------------------------------------------------------------------------
# Character
# -----------------------------------------------------------------------------

MACRO = {"gender": 1.0, "age": 0.5, "muscle": 0.5, "weight": 0.55, "proportions": 0.5, "height": 0.7,
         "cupsize": 0.5, "firmness": 0.5, "race": {"asian": 1.0, "caucasian": 0.0, "african": 0.0}}
SKIN = "young_asian_male.mhmat"
# (asset subdir, mhclo, MPFB asset type, Unreal slot, part)
ASSETS = [
    ("eyes", "high-poly.mhclo", "Eyes", "Eye", "head"),
    ("eyebrows", "eyebrow001.mhclo", "Eyebrows", "Eyebrows", "head"),
    ("eyelashes", "eyelashes01.mhclo", "Eyelashes", "Eyelashes", "head"),
    ("hair", "short02.mhclo", "Hair", "Hair", "head"),
    ("clothes", "male_elegantsuit01.mhclo", "Clothes", "Suit", "body"),
    ("clothes", "shoes04.mhclo", "Clothes", "Shoes", "body"),
]
# Unreal slot -> (exported texture name, max size, has alpha)
TEXTURES = {
    "Skin": ("T_OfficeWorker_Skin", 1024, False),
    "Eye": ("T_OfficeWorker_Eye", 512, False),   # the transparent cornea faces are removed
    "Eyebrows": ("T_OfficeWorker_Eyebrows", 256, True),
    "Eyelashes": ("T_OfficeWorker_Eyelashes", 256, True),
    "Hair": ("T_OfficeWorker_Hair", 1024, True),
    "Suit": ("T_OfficeWorker_Suit", 1024, False),
    "Shoes": ("T_OfficeWorker_Shoes", 512, False),
}

# Seated pose, bone directions in the character's frame before it is turned to face Unreal +X:
# the MakeHuman character faces Blender -Y, its left side is +X.  side = +1 for _l bones, -1 for _r.
POSE = [
    ("thigh", lambda s: (0.13 * s, -1.0, 0.0)),
    ("calf", lambda s: (0.02 * s, -0.22, -1.0)),
    ("foot", lambda s: (0.05 * s, -1.0, -0.45)),
    ("upperarm", lambda s: (0.20 * s, -0.42, -1.0)),
    ("lowerarm", lambda s: (-0.05 * s, -1.0, -0.42)),
    ("hand", lambda s: (-0.02 * s, -1.0, -0.75)),
]
SPINE = [("spine_01", -4.0), ("spine_03", 3.0), ("neck_01", 4.0)]   # forward tilt in degrees (+ = lean forward)

EYE_ABOVE_FLOOR = 121.0  # cm, MP_Main eye above the L_Office floor
SOLE_CLEARANCE = 0.4     # cm above the carpet
# AOfficePawn::_HeadPivot: the head mesh turns about this point (Unreal cm from the eyes, about the middle of the neck
# near the collar), so the neck stays inside the collar when the player looks around.
HEAD_PIVOT = (-10.0, 0.0, -13.0)
# Skin kept inside the collar on the body (Unreal cm box, shrunk 3 mm inwards) so that no gap opens between the collar
# and the neck when the head turns; at rest the head's own neck covers it.
NECK_FILLER = ((-18.0, -9.0, -26.0), (0.0, 9.0, -12.5))


def build_character():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    basemesh = HumanService.create_human(macro_detail_dict=MACRO)
    HumanService.set_character_skin(AssetService.find_asset_absolute_path(SKIN, asset_subdir="skins"), basemesh,
                                    skin_type="GAMEENGINE")
    rig = HumanService.add_builtin_rig(basemesh, "game_engine")
    parts = {}
    for sub, fn, atype, slot, part in ASSETS:
        path = AssetService.find_asset_absolute_path(fn, asset_subdir=sub)
        if path is None:
            raise RuntimeError(f"{fn} not found: install the makehuman_system_assets pack (README.md)")
        before = set(bpy.data.objects)
        HumanService.add_mhclo_asset(path, basemesh, asset_type=atype, material_type="GAMEENGINE", subdiv_levels=0)
        obj = [o for o in set(bpy.data.objects) - before if o.type == "MESH"][0]
        parts[slot] = (obj, part)
    return basemesh, rig, parts


# -----------------------------------------------------------------------------
# Pose
# -----------------------------------------------------------------------------


def aim(rig, pb, direction):
    bpy.context.view_layer.update()
    cur = (pb.tail - pb.head).normalized()
    to_arm = rig.matrix_world.inverted().to_3x3()
    q = cur.rotation_difference((to_arm @ Vector(direction)).normalized())
    h = pb.head.copy()
    pb.matrix = Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h) @ pb.matrix
    bpy.context.view_layer.update()


def tilt(rig, pb, degrees):
    """Rotate a bone about the character's left-right axis (+ = forward)."""
    bpy.context.view_layer.update()
    h = pb.head.copy()
    axis = (rig.matrix_world.inverted().to_3x3() @ Vector((1.0, 0.0, 0.0))).normalized()
    rot = Matrix.Rotation(math.radians(-degrees), 4, axis)
    pb.matrix = Matrix.Translation(h) @ rot @ Matrix.Translation(-h) @ pb.matrix
    bpy.context.view_layer.update()


def pose(rig, thigh_drop=None):
    for pb in rig.pose.bones:
        pb.matrix_basis = Matrix()
    bpy.context.view_layer.update()
    for name, deg in SPINE:
        tilt(rig, rig.pose.bones[name], deg)
    for base, fn in POSE:
        for suffix, side in (("_l", 1.0), ("_r", -1.0)):
            d = list(fn(side))
            if base == "thigh" and thigh_drop is not None:
                d[2] = -thigh_drop
            aim(rig, rig.pose.bones[base + suffix], d)


def evaluated_copy(obj, name):
    dg = bpy.context.evaluated_depsgraph_get()
    mesh = bpy.data.meshes.new_from_object(obj.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)
    copy = bpy.data.objects.new(name, mesh)
    copy.matrix_world = obj.matrix_world.copy()
    bpy.context.scene.collection.objects.link(copy)
    return copy


def lowest_z(objs):
    dg = bpy.context.evaluated_depsgraph_get()
    z = 1e9
    for o in objs:
        ev = o.evaluated_get(dg)
        m = ev.to_mesh()
        mw = o.matrix_world
        z = min(z, min((mw @ v.co).z for v in m.vertices))
        ev.to_mesh_clear()
    return z


def eye_center(eyes_obj):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = eyes_obj.evaluated_get(dg)
    m = ev.to_mesh()
    pts = [eyes_obj.matrix_world @ v.co for v in m.vertices]
    ev.to_mesh_clear()
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return (lo + hi) * 0.5, hi - lo


def solve_pose(rig, basemesh, parts):
    """Pick the thigh slope so that the soles rest on the floor EYE_ABOVE_FLOOR below the eyes."""
    shoes = parts["Shoes"][0]
    eyes = parts["Eye"][0]
    lo, hi = -0.3, 0.6
    for _ in range(16):
        mid = (lo + hi) * 0.5
        pose(rig, mid)
        eye, _ = eye_center(eyes)
        drop = (eye.z - lowest_z([shoes])) * 100.0
        if drop > EYE_ABOVE_FLOOR - SOLE_CLEARANCE:
            hi = mid   # feet too low: raise the knees
        else:
            lo = mid
    pose(rig, (lo + hi) * 0.5)
    eye, _ = eye_center(eyes)
    print(f"POSE thigh drop {(lo + hi) * 0.5:.3f}, eyes to soles {(eye.z - lowest_z([shoes])) * 100.0:.1f} cm")
    return eye


# -----------------------------------------------------------------------------
# Bake, split, export
# -----------------------------------------------------------------------------


def slot_material(name, image_path, alpha):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(str(image_path), check_existing=True)
    nt.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    if alpha:
        nt.links.new(tex.outputs["Alpha"], bsdf.inputs["Alpha"])
        mat.surface_render_method = "DITHERED"
    bsdf.inputs["Roughness"].default_value = 0.6
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    nt.nodes.active = tex
    return mat


def source_image(obj):
    for mat in obj.data.materials:
        if mat and mat.node_tree:
            for n in mat.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image and "normal" not in n.image.name.lower():
                    return n.image.filepath_from_user()
    raise RuntimeError(f"no diffuse texture on {obj.name}")


def copy_texture(src, slot):
    """Copy a MakeHuman texture into Textures/, box-filtered down to the size in TEXTURES."""
    import numpy as np
    name, size, alpha = TEXTURES[slot]
    TEX_DIR.mkdir(parents=True, exist_ok=True)
    print("TEXTURE", slot, src)
    img = bpy.data.images.load(src, check_existing=False)
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    px = px.reshape(h, w, 4)
    f = max(1, max(w, h) // size)
    if f > 1:
        px = px[:h // f * f, :w // f * f].reshape(h // f, f, w // f, f, 4).mean(axis=(1, 3))
    if not alpha:
        px[..., 3] = 1.0
    oh, ow = px.shape[:2]
    out = bpy.data.images.new("_tex_" + name, ow, oh, alpha=alpha)
    out.pixels.foreach_set(np.ascontiguousarray(px, dtype=np.float32).ravel())
    dst = TEX_DIR / f"{name}.png"
    out.filepath_raw = str(dst)
    out.file_format = "PNG"
    out.save()
    bpy.data.images.remove(out)
    return dst


def remove_transparent_faces(obj, image_path):
    """The eye texture leaves the cornea shell transparent; drop those faces so the eyes can stay opaque."""
    img = bpy.data.images.load(image_path, check_existing=False)
    w, h = img.size
    px = img.pixels[:]
    bpy.data.images.remove(img)
    uv = obj.data.uv_layers.active.data
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    layer = bm.loops.layers.uv.active
    drop = []
    for f in bm.faces:
        u = sum(l[layer].uv.x for l in f.loops) / len(f.loops)
        v = sum(l[layer].uv.y for l in f.loops) / len(f.loops)
        x = min(w - 1, max(0, int(u % 1.0 * w)))
        y = min(h - 1, max(0, int(v % 1.0 * h)))
        if px[(y * w + x) * 4 + 3] < 0.5:
            drop.append(f)
    bmesh.ops.delete(bm, geom=drop, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()
    print(f"EYE removed {len(drop)} transparent cornea faces")


def neck_filler(basemesh, to_ue, skin_mat):
    """Skin of the lower neck and upper chest that the clothes normally hide, shrunk 3 mm inwards."""
    masks = [m for m in basemesh.modifiers if m.type == "MASK" and m.name.startswith("Delete.")]
    for m in masks:
        m.show_viewport = False
    full = evaluated_copy(basemesh, "_neckfill")
    for m in masks:
        m.show_viewport = True
    full.data.transform(to_ue @ full.matrix_world)
    full.matrix_world = to_ue.inverted()
    lo, hi = Vector(NECK_FILLER[0]) / 100.0, Vector(NECK_FILLER[1]) / 100.0
    bm = bmesh.new()
    bm.from_mesh(full.data)
    def inside(f):
        c = f.calc_center_median()
        c = Vector((c.x, -c.y, c.z))  # Blender Y = -Unreal Y
        return all(lo[i] <= c[i] <= hi[i] for i in range(3))
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not inside(f)], context="FACES")
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bm.normal_update()
    for v in bm.verts:
        v.co -= v.normal * 0.003
    bm.to_mesh(full.data)
    bm.free()
    full.data.transform(full.matrix_world)
    full.matrix_world = Matrix()
    full.data.materials.clear()
    full.data.materials.append(skin_mat)
    for p in full.data.polygons:
        p.material_index = 0
    return full


def bake(basemesh, rig, parts, eye):
    """Apply the pose, split the body into head (head + neck groups) and body, turn to Unreal +X, eye at origin."""
    to_ue = Matrix.Rotation(math.radians(90.0), 4, "Z") @ Matrix.Translation(-eye)
    for bone in ("neck_01", "head"):  # for choosing HEAD_PIVOT
        p = to_ue @ (rig.matrix_world @ rig.pose.bones[bone].head)
        print(f"BONE {bone}: Unreal cm ({p.x * 100.0:.1f}, {-p.y * 100.0:.1f}, {p.z * 100.0:.1f})")

    textures = {"Skin": copy_texture(source_image(basemesh), "Skin")}
    for slot, (obj, _) in parts.items():
        textures[slot] = copy_texture(source_image(obj), slot)
    mats = {slot: slot_material(slot, path, TEXTURES[slot][2]) for slot, path in textures.items()}

    pieces = {"head": [], "body": []}
    body = evaluated_copy(basemesh, "_body")
    # split the skin at the base of the neck by bone weights
    vg = {g.name: g.index for g in body.vertex_groups}
    bone_groups = {vg[b.name] for b in rig.data.bones if b.name in vg}
    head_groups = {vg[n] for n in ("head", "neck_01") if n in vg}
    bm = bmesh.new()
    bm.from_mesh(body.data)
    deform = bm.verts.layers.deform.active
    def head_weight(v):
        d = v[deform]
        total = sum(w for g, w in d.items() if g in bone_groups) or 1.0
        return sum(w for g, w in d.items() if g in head_groups) / total
    hw = {v.index: head_weight(v) for v in bm.verts}
    head_faces = [f for f in bm.faces if sum(hw[v.index] for v in f.verts) / len(f.verts) > 0.5]
    bm.faces.index_update()
    keep = {f.index for f in head_faces}
    head_bm = bm.copy()
    bmesh.ops.delete(bm, geom=head_faces, context="FACES")
    head_bm.faces.ensure_lookup_table()
    bmesh.ops.delete(head_bm, geom=[f for f in head_bm.faces if f.index not in keep], context="FACES")
    bm.to_mesh(body.data)
    bm.free()
    head_skin = bpy.data.objects.new("_headskin", bpy.data.meshes.new("_headskin"))
    head_bm.to_mesh(head_skin.data)
    head_bm.free()
    head_skin.matrix_world = body.matrix_world.copy()
    bpy.context.scene.collection.objects.link(head_skin)
    for o in (body, head_skin):
        o.data.materials.clear()
        o.data.materials.append(mats["Skin"])
        for p in o.data.polygons:
            p.material_index = 0
    pieces["body"].append(body)
    pieces["head"].append(head_skin)
    pieces["body"].append(neck_filler(basemesh, to_ue, mats["Skin"]))
    for slot, (obj, part) in parts.items():
        c = evaluated_copy(obj, "_" + slot)
        if slot == "Eye":
            remove_transparent_faces(c, source_image(obj))
        c.data.materials.clear()
        c.data.materials.append(mats[slot])
        for p in c.data.polygons:
            p.material_index = 0
        pieces[part].append(c)

    out = {}
    for part, objs in pieces.items():
        for o in objs:
            print(f"PIECE {part} {o.name} tris={sum(len(p.vertices) - 2 for p in o.data.polygons)}")
        name = "SM_OfficeWorker_" + part.capitalize()
        bpy.ops.object.select_all(action="DESELECT")
        for o in objs:
            o.select_set(True)
        bpy.context.view_layer.objects.active = objs[0]
        bpy.ops.object.join()
        obj = bpy.context.view_layer.objects.active
        obj.name = name
        obj.data.name = name + "_Mesh"
        obj.data.transform(to_ue @ obj.matrix_world)
        obj.matrix_world = Matrix()
        obj.vertex_groups.clear()
        for attr in [a.name for a in obj.data.attributes if a.name.startswith("mpfb") or a.name.startswith("seam")]:
            obj.data.attributes.remove(obj.data.attributes[attr])
        obj.data.shape_keys and obj.shape_key_clear()
        out[part] = obj
    # remove the MPFB character
    for o in [basemesh, rig] + [p[0] for p in parts.values()]:
        bpy.data.objects.remove(o, do_unlink=True)
    return out


def report(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [-v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    print(f"{obj.name}: UE bounds cm min=({min(xs)*100:.1f}, {min(ys)*100:.1f}, {min(zs)*100:.1f}) "
          f"max=({max(xs)*100:.1f}, {max(ys)*100:.1f}, {max(zs)*100:.1f}) tris={tris} "
          f"slots={[m.name for m in obj.data.materials]}")


def export_fbx(obj, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"}, use_mesh_modifiers=True,
        mesh_smooth_type="FACE", path_mode="STRIP", axis_forward="-Z", axis_up="Y",
        global_scale=1.0, apply_unit_scale=True, use_space_transform=True, bake_anim=False)


# -----------------------------------------------------------------------------
# Preview (seat, desk and monitor around the pawn, like L_Office MP_Main)
# -----------------------------------------------------------------------------

PAWN = (326.0, 167.0, -4.0, 85.0)
REFERENCE = [  # fbx, L_Office world location cm, yaw
    (ART / "OfficeChair/Exports/SM_OfficeChair.fbx", (314.0, 175.0, -125.0), 0.0),
    (ART / "OfficeDesk/Exports/SM_OfficeDesk_A.fbx", (295.4, 299.7, -124.8), 180.0),
    (ART / "ComputerMonitor/Exports/SM_ComputerMonitor.fbx", (328.4, 267.7, -41.8), 180.0),
    (ART / "RetroComputer/Exports/SM_PCTower.fbx", (370.4, 267.7, -41.8), 180.0),
    (ART / "RetroComputer/Exports/SM_KeyboardStandard.fbx", (339.5, 213.5, -41.8), 180.0),
]


def pawn_local(x, y, z, yaw):
    px, py, pz, pyaw = PAWN
    a = math.radians(pyaw)
    dx, dy = x - px, y - py
    lx = math.cos(a) * dx + math.sin(a) * dy
    ly = -math.sin(a) * dx + math.cos(a) * dy
    return Vector((lx / 100.0, -ly / 100.0, (z - pz) / 100.0)), math.radians(-(yaw - pyaw))


def load_reference():
    objs = []
    for path, loc, yaw in REFERENCE:
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(path))
        p, r = pawn_local(*loc, yaw)
        for o in set(bpy.data.objects) - before:
            if o.type != "MESH":
                bpy.data.objects.remove(o, do_unlink=True)
                continue
            o.location = p
            o.rotation_euler = (0.0, 0.0, r)
            o.scale = (1.0, 1.0, 1.0)
            objs.append(o)
    return objs


def ue(x, y, z):
    return Vector((x / 100.0, -y / 100.0, z / 100.0))


def preview(out_dir, objs):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = 800, 800
    scene.render.film_transparent = False
    world = bpy.data.worlds.new("_PreviewWorld")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.35, 0.36, 0.38, 1.0)
    world.node_tree.nodes["Background"].inputs[1].default_value = 0.6
    scene.world = world
    light_data = bpy.data.lights.new("_Key", "AREA")
    light_data.energy = 220.0
    light_data.size = 1.0
    light = bpy.data.objects.new("_Key", light_data)
    light.location = ue(80.0, -60.0, 60.0)
    light.rotation_euler = (ue(0, 0, -30) - light.location).to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(light)
    refs = load_reference()
    cam_data = bpy.data.cameras.new("_PreviewCam")
    cam = bpy.data.objects.new("_PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    views = {
        # name: (Unreal cm location, look-at, lens, show references)
        "monitor": ((100.0, 6.0, -18.0), (0.0, 0.0, -12.0), 50.0, False),
        "face": ((42.0, 10.0, -2.0), (0.0, 0.0, -4.0), 50.0, False),
        "front": ((190.0, 30.0, -35.0), (0.0, 0.0, -55.0), 35.0, False),
        "side": ((15.0, 190.0, -45.0), (25.0, 0.0, -60.0), 30.0, True),
        "back": ((-110.0, -70.0, 5.0), (0.0, 0.0, -50.0), 35.0, False),
    }
    for name, (loc, tgt, lens, show_refs) in views.items():
        for r in refs:
            r.hide_render = not show_refs
        a, b = ue(*loc), ue(*tgt)
        cam_data.lens = lens
        cam.location = a
        cam.rotation_euler = (b - a).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(out_dir / f"worker_{name}.png")
        bpy.ops.render.render(write_still=True)
    for o in refs + [cam, light]:
        bpy.data.objects.remove(o, do_unlink=True)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    basemesh, rig, parts = build_character()
    eye = solve_pose(rig, basemesh, parts)
    objs = bake(basemesh, rig, parts, eye)
    for o in objs.values():
        report(o)
    if "--preview" in argv:
        preview(argv[argv.index("--preview") + 1], list(objs.values()))
    if "--no-save" not in argv:
        bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
        for o in objs.values():
            export_fbx(o, EXPORT_DIR / f"{o.name}.fbx")
        print(f"Saved {BLEND}")


main()
