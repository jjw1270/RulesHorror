"""Night 'ghost photos' for the in-game i-wish homepage (banner + member photo gallery).

Headless Blender with the MPFB extension (MakeHuman, CC0 assets; see Art/Blender/OfficeWorker/README.md):
    blender --background --python make_iwish_photos.py -- <scene> [--out <dir>] [--samples N]
scene: window | banner | corner | forest | wrist | all
Writes raw renders <out>/<scene>.png (+ <scene>.json with projected points). make_iwish_images.py turns them into
low-res 2004 digital camera photos.
"""
import importlib
import json
import math
import random
import sys
from pathlib import Path

import bpy
from bpy_extras.object_utils import world_to_camera_view
from mathutils import Matrix, Vector

HERE = Path(__file__).resolve().parent


def dyn(pkg, key):
    for name in list(sys.modules):
        if name.endswith(pkg):
            return getattr(importlib.import_module(name), key)
    raise RuntimeError(f"MPFB module {pkg} not found: install the MPFB extension")


HumanService = dyn("mpfb.services.humanservice", "HumanService")
AssetService = dyn("mpfb.services.assetservice", "AssetService")

# ---------------------------------------------------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------------------------------------------------


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)


def mat(name, color, rough=0.8, emit=None, emit_strength=0.0, spec=0.3, bump=None, wet=None):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    b = nt.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1.0)
    b.inputs["Roughness"].default_value = rough
    b.inputs["Specular IOR Level"].default_value = spec
    if emit:
        b.inputs["Emission Color"].default_value = (*emit, 1.0)
        b.inputs["Emission Strength"].default_value = emit_strength
    if bump:
        # procedural grime: noise texture -> base color variation + bump
        scale, amount, var = bump
        tc = nt.nodes.new("ShaderNodeTexCoord")
        nz = nt.nodes.new("ShaderNodeTexNoise")
        nz.inputs["Scale"].default_value = scale
        nz.inputs["Detail"].default_value = 8.0
        nt.links.new(tc.outputs["Object"], nz.inputs["Vector"])
        mix = nt.nodes.new("ShaderNodeMix")
        mix.data_type = "RGBA"
        mix.blend_type = "MULTIPLY"
        mix.inputs["Factor"].default_value = var
        mix.inputs[6].default_value = (*color, 1.0)
        nt.links.new(nz.outputs["Color"], mix.inputs[7])
        nt.links.new(mix.outputs[2], b.inputs["Base Color"])
        bp = nt.nodes.new("ShaderNodeBump")
        bp.inputs["Strength"].default_value = amount
        nt.links.new(nz.outputs["Fac"], bp.inputs["Height"])
        nt.links.new(bp.outputs["Normal"], b.inputs["Normal"])
    if wet:
        # puddles: low-frequency noise -> roughness ramp (wet = shiny)
        tc2 = nt.nodes.new("ShaderNodeTexCoord")
        nz2 = nt.nodes.new("ShaderNodeTexNoise")
        nz2.inputs["Scale"].default_value = wet
        nz2.inputs["Detail"].default_value = 3.0
        nt.links.new(tc2.outputs["Object"], nz2.inputs["Vector"])
        ramp = nt.nodes.new("ShaderNodeMapRange")
        ramp.inputs["From Min"].default_value = 0.45
        ramp.inputs["From Max"].default_value = 0.55
        ramp.inputs["To Min"].default_value = 0.05
        ramp.inputs["To Max"].default_value = rough
        nt.links.new(nz2.outputs["Fac"], ramp.inputs["Value"])
        nt.links.new(ramp.outputs["Result"], b.inputs["Roughness"])
    return m


def box(name, loc, size, material, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    o.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.data.materials.append(material)
    return o


def plane(name, loc, size, material, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_plane_add(size=1.0, location=loc, rotation=rot)
    o = bpy.context.active_object
    o.name = name
    o.scale = (size[0], size[1], 1.0)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.data.materials.append(material)
    return o


def cyl(name, loc, r, h, material, verts=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=r, depth=h, location=loc)
    o = bpy.context.active_object
    o.name = name
    o.data.materials.append(material)
    return o


def point_light(name, loc, energy, color, radius=0.1):
    d = bpy.data.lights.new(name, "POINT")
    d.energy = energy
    d.color = color
    d.shadow_soft_size = radius
    o = bpy.data.objects.new(name, d)
    o.location = loc
    bpy.context.collection.objects.link(o)
    return o


def area_light(name, loc, rot, size, energy, color):
    d = bpy.data.lights.new(name, "AREA")
    d.energy = energy
    d.color = color
    d.size = size[0]
    d.shape = "RECTANGLE"
    d.size_y = size[1]
    o = bpy.data.objects.new(name, d)
    o.location = loc
    o.rotation_euler = rot
    bpy.context.collection.objects.link(o)
    return o


def spot_light(name, loc, rot, energy, color, angle=70.0, blend=0.4, radius=0.05):
    d = bpy.data.lights.new(name, "SPOT")
    d.energy = energy
    d.color = color
    d.spot_size = math.radians(angle)
    d.spot_blend = blend
    d.shadow_soft_size = radius
    o = bpy.data.objects.new(name, d)
    o.location = loc
    o.rotation_euler = rot
    bpy.context.collection.objects.link(o)
    return o


def camera(loc, target, lens=28.0):
    d = bpy.data.cameras.new("Cam")
    d.lens = lens
    o = bpy.data.objects.new("Cam", d)
    o.location = loc
    bpy.context.collection.objects.link(o)
    direction = Vector(target) - Vector(loc)
    o.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = o
    return o


def world(color, strength):
    w = bpy.data.worlds.get("World") or bpy.data.worlds.new("World")
    bpy.context.scene.world = w
    w.use_nodes = True
    bg = w.node_tree.nodes["Background"]
    bg.inputs[0].default_value = (*color, 1.0)
    bg.inputs[1].default_value = strength


def fog(center, size, density, color=(0.6, 0.62, 0.65)):
    m = bpy.data.materials.new("Fog")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.remove(nt.nodes["Principled BSDF"])
    v = nt.nodes.new("ShaderNodeVolumePrincipled")
    v.inputs["Density"].default_value = density
    v.inputs["Color"].default_value = (*color, 1.0)
    nt.links.new(v.outputs[0], nt.nodes["Material Output"].inputs["Volume"])
    return box("FogVolume", center, size, m)


# ---------------------------------------------------------------------------------------------------------------------
# people (MPFB)
# ---------------------------------------------------------------------------------------------------------------------


def tex_material(name, image_path, tint=None, gray=False, alpha=False, rough=0.7):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    nt = m.node_tree
    b = nt.nodes["Principled BSDF"]
    b.inputs["Roughness"].default_value = rough
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = bpy.data.images.load(str(image_path), check_existing=True)
    col = t.outputs["Color"]
    if gray:
        bw = nt.nodes.new("ShaderNodeRGBToBW")
        nt.links.new(col, bw.inputs[0])
        col = bw.outputs[0]
    if tint:
        mx = nt.nodes.new("ShaderNodeMix")
        mx.data_type = "RGBA"
        mx.blend_type = "MULTIPLY"
        mx.inputs["Factor"].default_value = 1.0
        nt.links.new(col, mx.inputs[6])
        mx.inputs[7].default_value = (*tint, 1.0)
        col = mx.outputs[2]
    nt.links.new(col, b.inputs["Base Color"])
    if alpha:
        nt.links.new(t.outputs["Alpha"], b.inputs["Alpha"])
    return m


def diffuse_of(obj):
    for m in obj.data.materials:
        if m and m.node_tree:
            for n in m.node_tree.nodes:
                if n.type == "TEX_IMAGE" and n.image and "normal" not in n.image.name.lower() \
                        and "_ao" not in n.image.name.lower():
                    return n.image.filepath_from_user()
    return None


def person(macro, skin, parts, looks):
    """parts: [(subdir, mhclo, type)], looks: {mhclo: dict(tint=..., gray=..., alpha=...)}. Returns (rig, objs)."""
    base = HumanService.create_human(macro_detail_dict=macro)
    HumanService.set_character_skin(AssetService.find_asset_absolute_path(skin, asset_subdir="skins"), base,
                                    skin_type="GAMEENGINE")
    rig = HumanService.add_builtin_rig(base, "game_engine")
    objs = [base]
    skin_img = diffuse_of(base)
    sk = looks.get("skin", {})
    base.data.materials.clear()
    base.data.materials.append(tex_material("Skin", skin_img, tint=sk.get("tint"), rough=0.55))
    for sub, fn, atype in parts:
        path = AssetService.find_asset_absolute_path(fn, asset_subdir=sub)
        before = set(bpy.data.objects)
        HumanService.add_mhclo_asset(path, base, asset_type=atype, material_type="GAMEENGINE", subdiv_levels=0)
        obj = [o for o in set(bpy.data.objects) - before if o.type == "MESH"][0]
        img = diffuse_of(obj)
        lk = looks.get(fn, {})
        obj.data.materials.clear()
        obj.data.materials.append(tex_material("M_" + fn, img, tint=lk.get("tint"), gray=lk.get("gray", False),
                                               alpha=atype in ("Hair", "Eyebrows", "Eyelashes"), rough=lk.get("rough", 0.8)))
        objs.append(obj)
    return rig, objs


def aim(rig, pb, direction):
    bpy.context.view_layer.update()
    cur = (pb.tail - pb.head).normalized()
    to_arm = rig.matrix_world.inverted().to_3x3()
    q = cur.rotation_difference((to_arm @ Vector(direction)).normalized())
    h = pb.head.copy()
    pb.matrix = Matrix.Translation(h) @ q.to_matrix().to_4x4() @ Matrix.Translation(-h) @ pb.matrix
    bpy.context.view_layer.update()


def pose(rig, bones):
    """bones: [(name, (x,y,z))] directions in the character frame (faces -Y, left = +X), applied in order."""
    for name, d in bones:
        aim(rig, rig.pose.bones[name], d)


def arms_down(s_out=0.10, fwd=0.0):
    out = []
    for suf, s in (("_l", 1.0), ("_r", -1.0)):
        out += [("upperarm" + suf, (s_out * s, -fwd, -1.0)), ("lowerarm" + suf, (0.04 * s, -0.08 - fwd, -1.0)),
                ("hand" + suf, (0.02 * s, -0.05, -1.0))]
    return out


def place(rig, loc, yaw_deg):
    rig.location = loc
    rig.rotation_euler = (0.0, 0.0, math.radians(yaw_deg))
    bpy.context.view_layer.update()


MAN = {"gender": 1.0, "age": 0.6, "muscle": 0.5, "weight": 0.5, "proportions": 0.5, "height": 0.6,
       "cupsize": 0.5, "firmness": 0.5, "race": {"asian": 1.0, "caucasian": 0.0, "african": 0.0}}
WOMAN = {"gender": 0.0, "age": 0.45, "muscle": 0.3, "weight": 0.35, "proportions": 0.6, "height": 0.45,
         "cupsize": 0.4, "firmness": 0.5, "race": {"asian": 1.0, "caucasian": 0.0, "african": 0.0}}
EYES = [("eyes", "high-poly.mhclo", "Eyes"), ("eyebrows", "eyebrow001.mhclo", "Eyebrows"),
        ("eyelashes", "eyelashes01.mhclo", "Eyelashes")]


def suit_man():
    parts = EYES + [("hair", "short02.mhclo", "Hair"), ("clothes", "male_elegantsuit01.mhclo", "Clothes"),
                    ("clothes", "shoes04.mhclo", "Clothes")]
    looks = {"male_elegantsuit01.mhclo": {"tint": (0.35, 0.35, 0.37), "gray": True},
             "short02.mhclo": {"tint": (0.25, 0.25, 0.25), "gray": True}}
    return person(MAN, "middleage_asian_male.mhmat", parts, looks)


def pale_woman():
    parts = EYES + [("hair", "long01.mhclo", "Hair"), ("clothes", "female_casualsuit01.mhclo", "Clothes")]
    looks = {"female_casualsuit01.mhclo": {"tint": (1.6, 1.6, 1.55), "gray": True, "rough": 0.9},
             "long01.mhclo": {"tint": (0.12, 0.11, 0.11), "gray": True},
             "skin": {"tint": (0.95, 0.97, 1.0)}}
    return person(WOMAN, "young_asian_female.mhmat", parts, looks)


# ---------------------------------------------------------------------------------------------------------------------
# render
# ---------------------------------------------------------------------------------------------------------------------


def setup_render(w, h, samples):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    sc.cycles.samples = samples
    sc.cycles.use_denoising = True
    sc.render.resolution_x = w
    sc.render.resolution_y = h
    sc.render.resolution_percentage = 100
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.look = "None"
    sc.render.image_settings.file_format = "PNG"
    try:
        prefs = bpy.context.preferences.addons["cycles"].preferences
        for kind in ("OPTIX", "CUDA"):
            try:
                prefs.compute_device_type = kind
                prefs.get_devices()
                if any(d.type == kind for d in prefs.devices):
                    for d in prefs.devices:
                        d.use = d.type == kind
                    sc.cycles.device = "GPU"
                    break
            except Exception:
                pass
    except Exception:
        pass


def project(points):
    sc = bpy.context.scene
    cam = sc.camera
    out = {}
    for k, p in points.items():
        v = world_to_camera_view(sc, cam, Vector(p))
        out[k] = [v.x * sc.render.resolution_x, (1.0 - v.y) * sc.render.resolution_y, v.z]
    return out


def bone_head(rig, name):
    return rig.matrix_world @ rig.pose.bones[name].head


def bone_point(rig, name, t):
    pb = rig.pose.bones[name]
    return rig.matrix_world @ pb.head.lerp(pb.tail, t)


# ---------------------------------------------------------------------------------------------------------------------
# scenes
# ---------------------------------------------------------------------------------------------------------------------


def street(fig_loc, fig_yaw, look_up=True):
    """Night alley seen from a 2nd floor room: window frame, wet road, sodium street lamp, a man looking up."""
    world((0.004, 0.006, 0.012), 1.0)
    asphalt = mat("Asphalt", (0.05, 0.05, 0.05), rough=0.6, bump=(30.0, 0.3, 0.6), wet=0.35)
    concrete = mat("Concrete", (0.32, 0.31, 0.29), rough=0.9, bump=(8.0, 0.25, 0.5))
    brick = mat("Brick", (0.22, 0.12, 0.09), rough=0.9, bump=(14.0, 0.3, 0.5))
    metal = mat("Pole", (0.18, 0.18, 0.18), rough=0.5)
    frame = mat("Frame", (0.55, 0.55, 0.52), rough=0.5)
    wall_in = mat("RoomWall", (0.5, 0.47, 0.42), rough=0.9)
    plane("Road", (0, -10, 0), (40, 30), asphalt)
    box("Curb", (0, -2.0, 0.08), (40, 0.4, 0.16), concrete)
    box("Sidewalk", (0, -0.9, 0.06), (40, 1.8, 0.12), concrete)
    box("FarWall", (0, -15.5, 1.1), (40, 0.3, 2.2), concrete)
    box("House1", (-7, -21, 3.5), (9, 8, 7), brick)
    box("House2", (6.5, -22, 2.8), (8, 8, 5.6), concrete)
    lit = mat("LitWindow", (0.1, 0.1, 0.1), emit=(1.0, 0.75, 0.45), emit_strength=2.5)
    box("HouseWin", (-5.5, -16.95, 4.2), (1.0, 0.05, 0.8), lit)
    # street lamp
    cyl("LampPole", (3.6, -13.5, 2.6), 0.08, 5.2, metal)
    box("LampArm", (3.0, -13.5, 5.15), (1.3, 0.1, 0.08), metal)
    head = mat("LampHead", (0.1, 0.1, 0.1), emit=(1.0, 0.55, 0.2), emit_strength=25.0)
    box("LampHeadMesh", (2.45, -13.5, 5.08), (0.4, 0.22, 0.06), head)
    spot_light("Lamp", (2.45, -13.5, 5.0), (0, 0, 0), 900.0, (1.0, 0.55, 0.22), angle=95.0, blend=0.7, radius=0.15)
    pole = mat("UtilityPole", (0.35, 0.34, 0.32), rough=0.9, bump=(10.0, 0.2, 0.3))
    cyl("UtilityPole", (-3.2, -9.8, 4.5), 0.14, 9.0, pole)
    box("PoleBar", (-3.2, -9.8, 8.2), (1.4, 0.1, 0.1), pole)
    wire = mat("Wire", (0.02, 0.02, 0.02), rough=0.6)
    for i, (dz, sag) in enumerate(((8.15, 0.5), (7.7, 0.7), (7.3, 0.6))):
        bpy.ops.curve.primitive_bezier_curve_add(location=(0, 0, 0))
        c = bpy.context.active_object
        c.data.bevel_depth = 0.012
        sp = c.data.splines[0]
        sp.bezier_points[0].co = (-3.2 - 0.5 + i * 0.5, -9.8, dz)
        sp.bezier_points[1].co = (14.0, -9.0 - i * 1.5, dz + 0.5)
        for bp_ in sp.bezier_points:
            bp_.handle_left_type = bp_.handle_right_type = "AUTO"
        sp.bezier_points[0].handle_right = (3.0, -9.6, dz - sag * 3)
        sp.bezier_points[1].handle_left = (8.0, -9.3, dz - sag * 3)
        c.data.materials.append(wire)
    # the room: wall with a window opening (2nd floor, sill at 4.9 m)
    zf = 4.0
    box("WallBelow", (0, 0.3, zf + 0.45), (8, 0.2, 0.9 + 0.0), wall_in)
    box("WallAbove", (0, 0.3, zf + 2.6), (8, 0.2, 0.8), wall_in)
    box("WallL", (-2.6, 0.3, zf + 1.5), (3.6, 0.2, 2.2), wall_in)
    box("WallR", (2.6, 0.3, zf + 1.5), (3.6, 0.2, 2.2), wall_in)
    # sliding window frames (two sashes) + middle rail
    for x in (-0.8, 0.0, 0.8):
        box("Mullion%.1f" % x, (x, 0.28, zf + 1.5), (0.06, 0.08, 1.4), frame)
    box("Sill", (0, 0.28, zf + 0.83), (1.7, 0.1, 0.06), frame)
    box("Head", (0, 0.28, zf + 2.17), (1.7, 0.1, 0.06), frame)
    # room light spill (very weak) so the frame reads
    point_light("Room", (0.3, 2.6, zf + 1.8), 3.0, (1.0, 0.85, 0.7), radius=0.5)
    spill = Vector(fig_loc) + Vector((0, 0, 1.4)) - Vector((0.3, 0.6, zf + 1.6))
    spot_light("RoomSpill", (0.3, 0.6, zf + 1.6), spill.to_track_quat("-Z", "Y").to_euler(), 500.0, (1.0, 0.82, 0.62),
               angle=18.0, blend=0.8, radius=0.4)
    fog((0, -10, 3), (30, 16, 6), 0.012, (0.8, 0.75, 0.7))
    rig, objs = suit_man()
    bones = arms_down(0.12, 0.02)
    if look_up:
        bones += [("neck_01", (0.0, 0.2, 1.0)), ("head", (0.0, 0.75, 1.0))]   # top of the head goes back = looks up
    pose(rig, bones)
    place(rig, fig_loc, fig_yaw)
    return rig


def scene_window(samples):
    setup_render(800, 600, samples)
    rig = street((1.6, -11.2, 0.0), 180.0)
    camera((0.5, 1.0, 5.5), (1.5, -11.2, 1.1), lens=55.0)
    return {"face": bone_point(rig, "head", 0.5)}


def scene_banner(samples):
    setup_render(1175, 240, samples)
    rig = street((1.6, -11.2, 0.0), 180.0)
    camera((0.2, 1.0, 5.5), (0.4, -11.0, 1.2), lens=32.0)
    return {"face": bone_point(rig, "head", 0.5)}


def scene_corner(samples):
    """Night school corridor: lower walls painted green, one tube light on at the end, a girl facing the corner."""
    setup_render(800, 600, samples)
    world((0.002, 0.003, 0.006), 1.0)
    floor = mat("Floor", (0.24, 0.25, 0.22), rough=0.25, bump=(12.0, 0.1, 0.4))
    upper = mat("WallUpper", (0.5, 0.49, 0.44), rough=0.9, bump=(5.0, 0.15, 0.55))
    lower = mat("WallLower", (0.16, 0.26, 0.2), rough=0.5, bump=(6.0, 0.1, 0.25))
    ceil = mat("Ceil", (0.6, 0.6, 0.58), rough=0.9)
    dark_glass = mat("NightGlass", (0.01, 0.012, 0.02), rough=0.05, spec=0.8)
    L = 22.0
    plane("Floor", (0, -L / 2, 0), (2.6, L), floor)
    plane("Ceiling", (0, -L / 2, 2.8), (2.6, L), ceil, rot=(math.pi, 0, 0))
    for sx in (-1, 1):
        box("Lower%d" % sx, (1.32 * sx, -L / 2, 0.55), (0.04, L, 1.1), lower)
        box("Upper%d" % sx, (1.32 * sx, -L / 2, 1.95), (0.04, L, 1.7), upper)
    # windows on the right wall (dark night glass)
    for i in range(6):
        box("Win%d" % i, (1.29, -2.5 - i * 3.4, 1.85), (0.02, 2.0, 1.1), dark_glass)
    box("EndWall", (0, -L, 1.4), (2.7, 0.1, 2.8), upper)
    box("EndLower", (0, -L + 0.06, 0.55), (2.7, 0.02, 1.1), lower)
    # tube lights: only the last one is on
    tube_on = mat("TubeOn", (0.9, 0.95, 1.0), emit=(0.85, 0.95, 1.0), emit_strength=6.0)
    tube_off = mat("TubeOff", (0.2, 0.2, 0.2), rough=0.6)
    for i in range(7):
        y = -1.5 - i * 3.2
        box("Tube%d" % i, (0, y, 2.77), (0.14, 1.2, 0.04), tube_on if i == 6 else tube_off)
    area_light("TubeLight", (0, -1.5 - 6 * 3.2, 2.7), (0, 0, 0), (0.2, 1.2), 45.0, (0.85, 0.95, 1.0))
    exit_sign = mat("Exit", (0.0, 0.2, 0.05), emit=(0.1, 1.0, 0.3), emit_strength=3.0)
    box("ExitSign", (-1.29, -L + 1.0, 2.35), (0.03, 0.5, 0.18), exit_sign)
    rig, objs = pale_woman()
    # facing the corner: head bowed against the wall
    pose(rig, arms_down(0.06, 0.0) + [("neck_01", (0.0, -0.3, 1.0)), ("head", (0.0, -0.55, 1.0))])
    place(rig, (0.85, -L + 0.45, 0.0), 0.0)   # faces -Y = towards the end wall, back to the camera
    camera((-0.45, -L + 9.5, 1.55), (0.45, -L, 1.0), lens=42.0)
    return {"head": bone_point(rig, "head", 0.5)}


def scene_forest(samples):
    """Camera flash in a night forest: pale trunks nearby, fog, a woman standing far between the trees."""
    setup_render(800, 600, samples)
    world((0.0, 0.0, 0.0), 0.0)
    random.seed(7)
    ground = mat("Leaves", (0.09, 0.065, 0.04), rough=0.9, bump=(25.0, 0.6, 0.8))
    barks = [mat("Bark%d" % i, c, rough=0.9, bump=(9.0 + i * 3, 0.9, 0.8)) for i, c in
             enumerate(((0.2, 0.17, 0.14), (0.13, 0.11, 0.09), (0.26, 0.24, 0.21), (0.1, 0.09, 0.08)))]
    plane("Ground", (0, -15, 0), (60, 60), ground)
    for i in range(70):
        x = random.uniform(-14, 14)
        y = random.uniform(-30, -2.5)
        if abs(x - 0.9) < 1.0 and -10 < y < -7:
            continue   # keep the gap where she stands
        if abs(x) < 0.7 and y > -4:
            continue
        r = random.uniform(0.06, 0.3)
        b = barks[i % len(barks)]
        bpy.ops.mesh.primitive_cone_add(vertices=10, radius1=r * 1.25, radius2=r * 0.55, depth=15.0, location=(x, y, 7.4))
        o = bpy.context.active_object
        o.data.materials.append(b)
        o.rotation_euler = (random.uniform(-0.07, 0.07), random.uniform(-0.07, 0.07), random.uniform(0, 6))
    fog((0, -16, 3), (40, 30, 6), 0.02, (0.7, 0.72, 0.75))
    # flash: small strong light at the camera
    cam_loc = (0.0, 0.5, 1.6)
    spot_light("Flash", (0.12, 0.45, 1.68), (math.radians(-90), 0, 0), 500.0, (1.0, 0.97, 0.92), angle=140.0, blend=1.0, radius=0.02)
    rig, objs = pale_woman()
    pose(rig, arms_down(0.07, 0.0) + [("head", (0.0, -0.25, 1.0))])
    place(rig, (0.9, -8.6, 0.0), 180.0)   # faces the camera
    camera(cam_loc, (0.5, -8.6, 1.1), lens=30.0)
    return {"head": bone_point(rig, "head", 0.5)}


def scene_wrist(samples):
    """Flash close-up of an upturned inner wrist on a bed sheet, mark position projected for the 2D brand."""
    setup_render(800, 600, samples)
    world((0.0, 0.0, 0.0), 0.0)
    rig, objs = person(WOMAN, "young_asian_female.mhmat", [], {"skin": {"tint": (0.97, 0.95, 0.95)}})
    sheet = mat("Sheet", (0.36, 0.34, 0.32), rough=0.95, bump=(3.0, 0.5, 0.25))   # after MPFB (it purges unused materials)
    # left arm forward and down to the sheet, palm up (twist the forearm about its own axis)
    pose(rig, [("upperarm_l", (0.25, -0.75, -0.6)), ("lowerarm_l", (0.05, -1.0, -0.1)), ("hand_l", (0.0, -1.0, -0.05))])
    lo = rig.pose.bones["lowerarm_l"]
    bpy.context.view_layer.update()
    axis = (lo.tail - lo.head).normalized()
    h = lo.head.copy()
    tw = Matrix.Rotation(math.radians(-150), 4, axis)
    lo.matrix = Matrix.Translation(h) @ tw @ Matrix.Translation(-h) @ lo.matrix
    bpy.context.view_layer.update()
    place(rig, (0, 0, 0), 0.0)
    wrist = bone_point(rig, "lowerarm_l", 0.86)
    elbow = bone_point(rig, "lowerarm_l", 0.35)
    hand = bone_point(rig, "hand_l", 0.6)
    z = wrist.z - 0.045
    plane("Sheet", (wrist.x, wrist.y, z), (3, 3), sheet)
    # camera above the wrist, a bit to the outside
    cam = camera((wrist.x + 0.16, wrist.y + 0.05, wrist.z + 0.34), (wrist.x + 0.0, wrist.y - 0.02, wrist.z), lens=35.0)
    spot_light("Flash", cam.location + Vector((0.03, 0.0, 0.02)), cam.rotation_euler, 2.5, (1.0, 0.97, 0.93),
               angle=80.0, blend=0.3, radius=0.01)
    up = Vector((0, 0, 1))
    return {"wrist": wrist + up * 0.025, "elbow": elbow + up * 0.03, "hand": hand + up * 0.02}


SCENES = {"window": scene_window, "banner": scene_banner, "corner": scene_corner, "forest": scene_forest,
          "wrist": scene_wrist}


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    which = argv[0] if argv else "all"
    out = Path(argv[argv.index("--out") + 1]) if "--out" in argv else HERE / "Renders"
    samples = int(argv[argv.index("--samples") + 1]) if "--samples" in argv else 96
    out.mkdir(parents=True, exist_ok=True)
    names = list(SCENES) if which == "all" else which.split(",")
    for n in names:
        clear()
        pts = SCENES[n](samples)
        bpy.context.view_layer.update()
        bpy.context.scene.render.filepath = str(out / (n + ".png"))
        bpy.ops.render.render(write_still=True)
        (out / (n + ".json")).write_text(json.dumps(project({k: tuple(v) for k, v in pts.items()})))
        print("RENDERED", n)


main()
