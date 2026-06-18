import bpy
import math
import random
from pathlib import Path

# RulesHorror UE-ready folded newspaper asset generator.
# Clean rebuild: horizontal XY page plane, portrait full-open newspaper,
# three blank sheets, paper texture only, two unfold animation clips.

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "FoldedNewspaperPrototype.blend"
TEXTURE_DIR = BASE_DIR / "Textures"
EXPORT_DIR = BASE_DIR / "Exports"
TEXTURE_BASECOLOR = TEXTURE_DIR / "T_NewspaperPaper_Aged_BaseColor.png"
OUTPUT_FBX = EXPORT_DIR / "SK_Newspaper_3Sheets.fbx"

MESH_NAME = "SK_Newspaper_3Sheets"
ARMATURE_NAME = "ARM_Newspaper_3Sheets"
MATERIAL_NAME = "M_NewspaperPaper_AgedBlank"

FULL_WIDTH_M = 0.60
FULL_HEIGHT_M = 0.84
PAPER_THICKNESS_M = 0.00055
SHEET_GAP_M = 0.00135
SHEET_COUNT = 3

FRAME_FOLDED = 1
FRAME_FIRST_UNFOLD = 60
FRAME_FULL_OPEN = 120
CLIP_START = 1
CLIP_END = 60

ANIM_UNFOLD_01 = "AN_Unfold_01_FoldedToWideHalfOpen"
ANIM_UNFOLD_02 = "AN_Unfold_02_WideHalfOpenToFullOpen"
ANIM_FOLD_01 = "AN_Fold_01_FullOpenToWideHalfFolded"
ANIM_FOLD_02 = "AN_Fold_02_WideHalfFoldedToFolded"
ANIM_DEFAULT_FOLDED = "AN_Default_FoldedPose"
ANIM_PREVIEW = "AN_Preview_FullTwoStepUnfold"
ANIM_PREVIEW_FOLD = "AN_Preview_FullTwoStepFold"


# -----------------------------------------------------------------------------
# Scene / material
# -----------------------------------------------------------------------------


def ensure_dirs():
    TEXTURE_DIR.mkdir(parents=True, exist_ok=True)
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)


def clear_scene():
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    for collection in (
        bpy.data.meshes,
        bpy.data.armatures,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.actions,
        bpy.data.collections,
    ):
        for datablock in list(collection):
            if datablock.users == 0:
                collection.remove(datablock)


def set_scene():
    scene = bpy.context.scene
    scene.frame_start = FRAME_FOLDED
    scene.frame_end = FRAME_FULL_OPEN
    scene.frame_set(FRAME_FOLDED)
    scene.render.fps = 24
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.view_settings.view_transform = "Filmic"
    scene.view_settings.look = "Medium High Contrast"
    scene["AssetPurpose"] = "Blank 3-sheet portrait newspaper for UE WidgetComponent content."
    scene["CoordinatePlane"] = "Horizontal XY paper plane: X=width, Y=height, Z=thickness."
    scene["FoldOrder"] = "Fold01: long side 0.84 folds to 0.42; Fold02: remaining width folds to final quarter."
    scene["DefaultPose"] = "Two-step folded pose via AN_Default_FoldedPose at frame 1."
    scene["AnimationClips"] = f"{ANIM_DEFAULT_FOLDED}, {ANIM_UNFOLD_01}, {ANIM_UNFOLD_02}, {ANIM_FOLD_01}, {ANIM_FOLD_02}; previews={ANIM_PREVIEW}, {ANIM_PREVIEW_FOLD}"
    scene["ExportFBX"] = str(OUTPUT_FBX)
    scene["BaseColorTexture"] = str(TEXTURE_BASECOLOR)


def generate_paper_texture(path, size=1024):
    """Generate a blank but clearly newsprint-like paper texture.

    No readable newspaper content is drawn because the actual content is meant
    to be supplied in Unreal via WidgetComponent/material layering. The texture
    focuses on recycled gray paper, fibers, pulp flecks, faint ink transfer,
    subtle halftone dirt, edge aging, and fold wear.
    """
    random.seed(6202026)
    image = bpy.data.images.new(path.stem, width=size, height=size, alpha=True, float_buffer=False)
    pixels = [0.0] * (size * size * 4)

    stains = [
        (random.random(), random.random(), random.uniform(0.035, 0.16), random.uniform(0.015, 0.065))
        for _ in range(46)
    ]
    ghost_bands = [
        (random.uniform(0.08, 0.92), random.uniform(0.08, 0.92), random.uniform(0.035, 0.085), random.uniform(0.012, 0.035))
        for _ in range(36)
    ]
    for y in range(size):
        v = y / (size - 1)
        for x in range(size):
            u = x / (size - 1)

            # Base newsprint is cooler/grayer than aged parchment.
            grain = random.uniform(-0.035, 0.035)
            long_fiber = 0.020 * math.sin(u * 540.0 + math.sin(v * 33.0) * 2.5)
            cross_fiber = 0.010 * math.sin(v * 760.0 + math.sin(u * 21.0) * 3.0)
            micro_fiber = 0.008 * math.sin((u + v) * 1400.0)

            # Very subtle printer/halftone residue without readable text.
            halftone = 0.0
            dot_x = math.sin(u * size * 0.34)
            dot_y = math.sin(v * size * 0.34)
            if dot_x > 0.78 and dot_y > 0.78:
                halftone -= 0.018

            stain_value = 0.0
            for sx, sy, radius, strength in stains:
                dx = u - sx
                dy = v - sy
                stain_value += strength * math.exp(-(dx * dx + dy * dy) / (radius * radius))

            # Faint ink transfer / column ghosting, deliberately unreadable.
            ghost = 0.0
            for gx, gy, width, strength in ghost_bands:
                vertical = math.exp(-((u - gx) ** 2) / (width * width))
                horizontal_gate = 0.45 + 0.55 * math.sin((v - gy) * 95.0)
                ghost -= strength * vertical * max(0.0, horizontal_gate)

            # Sparse pulp flecks; kept subtle so WidgetComponent content can still read clearly.
            fleck = 0.0
            fleck_roll = random.random()
            if fleck_roll < 0.010:
                fleck -= random.uniform(0.025, 0.095)
            elif fleck_roll < 0.017:
                fleck += random.uniform(0.020, 0.070)

            # Edges and fold wear: darker gray/yellow at borders and fold lines.
            edge = min(u, 1.0 - u, v, 1.0 - v)
            edge_wear = 0.040 * math.exp(-edge / 0.035)
            fold_wear = 0.030 * math.exp(-((u - 0.5) ** 2) / 0.00055)
            fold_wear += 0.035 * math.exp(-((v - 0.5) ** 2) / 0.00065)

            shade = grain + long_fiber + cross_fiber + micro_fiber + halftone + ghost + fleck - stain_value - edge_wear - fold_wear

            # Newsprint palette: gray-beige, lower saturation, slightly dirty.
            r = 0.655 + shade * 0.95 + edge_wear * 0.25
            g = 0.645 + shade * 0.92 + edge_wear * 0.18
            b = 0.595 + shade * 0.80 - edge_wear * 0.05

            # Random peppering common in low-grade paper/ink residue.
            if random.random() < 0.010:
                pepper = random.uniform(0.06, 0.18)
                r -= pepper
                g -= pepper
                b -= pepper
            elif random.random() < 0.006:
                lint = random.uniform(0.04, 0.10)
                r += lint
                g += lint
                b += lint

            idx = (y * size + x) * 4
            pixels[idx + 0] = max(0.34, min(0.84, r))
            pixels[idx + 1] = max(0.34, min(0.82, g))
            pixels[idx + 2] = max(0.31, min(0.76, b))
            pixels[idx + 3] = 1.0

    image.pixels.foreach_set(pixels)
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    image.pack()
    return image


def find_node_by_type(nodes, bl_idname):
    for node in nodes:
        if node.bl_idname == bl_idname:
            return node
    return None


def make_material(image):
    material = bpy.data.materials.new(MATERIAL_NAME)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    bsdf = find_node_by_type(nodes, "ShaderNodeBsdfPrincipled")
    if bsdf is None:
        bsdf = nodes.new(type="ShaderNodeBsdfPrincipled")
    bsdf.inputs["Base Color"].default_value = (0.76, 0.73, 0.66, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.94
    bsdf.inputs["Metallic"].default_value = 0.0

    tex = nodes.new(type="ShaderNodeTexImage")
    tex.name = "T_NewspaperPaper_Aged_BaseColor"
    tex.label = tex.name
    tex.image = image
    material.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return material


# -----------------------------------------------------------------------------
# Mesh
# -----------------------------------------------------------------------------


def add_panel_box(verts, faces, uv_faces, vertex_bones, x0, x1, y0, y1, z, thickness, bone_name, uv_rect):
    start = len(verts)
    z0 = z - thickness * 0.5
    z1 = z + thickness * 0.5

    verts.extend([
        (x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
        (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1),
    ])
    vertex_bones.extend([bone_name] * 8)

    u0, v0, u1, v1 = uv_rect
    top_uv = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
    edge_uv = [(u0, v0), (u1, v0), (u1, v0 + 0.01), (u0, v0 + 0.01)]
    panel_faces = [
        ([start + 4, start + 5, start + 6, start + 7], top_uv),
        ([start + 3, start + 2, start + 1, start + 0], top_uv),
        ([start + 0, start + 1, start + 5, start + 4], edge_uv),
        ([start + 1, start + 2, start + 6, start + 5], edge_uv),
        ([start + 2, start + 3, start + 7, start + 6], edge_uv),
        ([start + 3, start + 0, start + 4, start + 7], edge_uv),
    ]
    for face, uv in panel_faces:
        faces.append(face)
        uv_faces.append(uv)


def create_mesh(material):
    verts = []
    faces = []
    uv_faces = []
    vertex_bones = []

    panel_specs = {
        "BL": (-FULL_WIDTH_M * 0.5, 0.0, -FULL_HEIGHT_M * 0.5, 0.0, (0.00, 0.00, 0.50, 0.50)),
        "BR": (0.0, FULL_WIDTH_M * 0.5, -FULL_HEIGHT_M * 0.5, 0.0, (0.50, 0.00, 1.00, 0.50)),
        "TL": (-FULL_WIDTH_M * 0.5, 0.0, 0.0, FULL_HEIGHT_M * 0.5, (0.00, 0.50, 0.50, 1.00)),
        "TR": (0.0, FULL_WIDTH_M * 0.5, 0.0, FULL_HEIGHT_M * 0.5, (0.50, 0.50, 1.00, 1.00)),
    }

    for sheet_index in range(1, SHEET_COUNT + 1):
        z = (sheet_index - 1) * SHEET_GAP_M
        prefix = f"Sheet_{sheet_index:02d}"

        # Stable two-step newspaper hierarchy:
        # - TR is the fixed reference quadrant.
        # - BR has its own horizontal bottom fold.
        # - TL belongs to one vertical left-column group.
        # - BL has its own horizontal bottom fold under that same left-column group.
        # This avoids twisting during the second fold because the already half-folded
        # left column rotates as one group.
        panel_bone = {
            "TR": f"{prefix}_Root_TR",
            "BR": f"{prefix}_Fold_H_Bottom_Right",
            "TL": f"{prefix}_Fold_V_Left_Group",
            "BL": f"{prefix}_Fold_H_Bottom_Left",
        }

        for panel_name, (x0, x1, y0, y1, uv_rect) in panel_specs.items():
            add_panel_box(
                verts,
                faces,
                uv_faces,
                vertex_bones,
                x0,
                x1,
                y0,
                y1,
                z,
                PAPER_THICKNESS_M,
                panel_bone[panel_name],
                uv_rect,
            )

    mesh = bpy.data.meshes.new(MESH_NAME + "_Mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update()

    uv_layer = mesh.uv_layers.new(name="NewspaperPageUV")
    for poly, uv in zip(mesh.polygons, uv_faces):
        for loop_index, loop_uv in zip(poly.loop_indices, uv):
            uv_layer.data[loop_index].uv = loop_uv

    mesh_obj = bpy.data.objects.new(MESH_NAME, mesh)
    bpy.context.collection.objects.link(mesh_obj)
    mesh_obj.data.materials.append(material)

    for bone_name in sorted(set(vertex_bones)):
        mesh_obj.vertex_groups.new(name=bone_name)
    for vertex_index, bone_name in enumerate(vertex_bones):
        mesh_obj.vertex_groups[bone_name].add([vertex_index], 1.0, "ADD")

    mesh_obj["UEAssetType"] = "SkeletalMesh"
    mesh_obj["ContentIntent"] = "Paper material only; use UE WidgetComponent/material layer for newspaper content."
    return mesh_obj


# -----------------------------------------------------------------------------
# Armature / animation
# -----------------------------------------------------------------------------


def create_armature():
    bpy.ops.object.armature_add(enter_editmode=True, location=(0, 0, 0))
    arm_obj = bpy.context.object
    arm_obj.name = ARMATURE_NAME
    arm_obj.data.name = ARMATURE_NAME + "_Data"
    arm_obj.show_in_front = True
    arm_data = arm_obj.data

    # Remove Blender's localized default bone (e.g. "Bone" / "??") so
    # the final skeleton contains only deliberate newspaper fold bones.
    for edit_bone in list(arm_data.edit_bones):
        arm_data.edit_bones.remove(edit_bone)

    root = arm_data.edit_bones.new("Root")
    root.head = (0.0, -0.05, -0.025)
    root.tail = (0.0, 0.075, -0.025)

    for sheet_index in range(1, SHEET_COUNT + 1):
        z = (sheet_index - 1) * SHEET_GAP_M
        prefix = f"Sheet_{sheet_index:02d}"

        sheet_root = arm_data.edit_bones.new(f"{prefix}_Root_TR")
        sheet_root.head = (0.0, 0.0, z)
        sheet_root.tail = (0.0, FULL_HEIGHT_M * 0.5, z)
        sheet_root.parent = root
        sheet_root.use_connect = False

        v_left_group = arm_data.edit_bones.new(f"{prefix}_Fold_V_Left_Group")
        v_left_group.head = (0.0, 0.0, z)
        v_left_group.tail = (0.0, FULL_HEIGHT_M * 0.5, z)
        v_left_group.parent = sheet_root
        v_left_group.use_connect = False

        h_bottom_right = arm_data.edit_bones.new(f"{prefix}_Fold_H_Bottom_Right")
        h_bottom_right.head = (0.0, 0.0, z)
        h_bottom_right.tail = (FULL_WIDTH_M * 0.5, 0.0, z)
        h_bottom_right.parent = sheet_root
        h_bottom_right.use_connect = False

        h_bottom_left = arm_data.edit_bones.new(f"{prefix}_Fold_H_Bottom_Left")
        h_bottom_left.head = (-FULL_WIDTH_M * 0.5, 0.0, z)
        h_bottom_left.tail = (0.0, 0.0, z)
        h_bottom_left.parent = v_left_group
        h_bottom_left.use_connect = False

        # Widget/socket helper bones. These are non-deforming bones intended as
        # Unreal attach targets for WidgetComponents or sockets. Use one widget
        # per quarter panel if the UI must follow folding animation.
        socket_specs = [
            (f"{prefix}_Socket_Widget_TR", ( FULL_WIDTH_M * 0.25,  FULL_HEIGHT_M * 0.25, z + PAPER_THICKNESS_M), sheet_root),
            (f"{prefix}_Socket_Widget_BR", ( FULL_WIDTH_M * 0.25, -FULL_HEIGHT_M * 0.25, z + PAPER_THICKNESS_M), h_bottom_right),
            (f"{prefix}_Socket_Widget_TL", (-FULL_WIDTH_M * 0.25,  FULL_HEIGHT_M * 0.25, z + PAPER_THICKNESS_M), v_left_group),
            (f"{prefix}_Socket_Widget_BL", (-FULL_WIDTH_M * 0.25, -FULL_HEIGHT_M * 0.25, z + PAPER_THICKNESS_M), h_bottom_left),
        ]
        for socket_name, socket_head, socket_parent in socket_specs:
            socket_bone = arm_data.edit_bones.new(socket_name)
            socket_bone.head = socket_head
            socket_bone.tail = (socket_head[0], socket_head[1], socket_head[2] + 0.035)
            socket_bone.parent = socket_parent
            socket_bone.use_connect = False

    bpy.ops.object.mode_set(mode="POSE")
    for pose_bone in arm_obj.pose.bones:
        pose_bone.rotation_mode = "XYZ"
    bpy.ops.object.mode_set(mode="OBJECT")
    arm_obj["UEAssetType"] = "Skeleton"
    arm_obj["FoldHierarchy"] = "Root_TR fixed; H_Bottom_Left/Right fold long side first; V_Left_Group folds left column second; Socket_Widget_* bones are attach helpers."
    return arm_obj


def bind_mesh(mesh_obj, arm_obj):
    mesh_obj.parent = arm_obj
    modifier = mesh_obj.modifiers.new("Armature_NewspaperFolds", "ARMATURE")
    modifier.object = arm_obj
    modifier.use_vertex_groups = True


def set_pose(arm_obj, h_bottom_degrees, v_left_degrees):
    for sheet_index in range(1, SHEET_COUNT + 1):
        prefix = f"Sheet_{sheet_index:02d}"
        # First fold: two horizontal bottom bones fold the long 0.84 side to 0.42.
        # Second fold: one left-column vertical group folds the 0.60 side to 0.30.
        arm_obj.pose.bones[f"{prefix}_Fold_H_Bottom_Right"].rotation_euler = (0.0, math.radians(h_bottom_degrees), 0.0)
        arm_obj.pose.bones[f"{prefix}_Fold_H_Bottom_Left"].rotation_euler = (0.0, math.radians(h_bottom_degrees), 0.0)
        arm_obj.pose.bones[f"{prefix}_Fold_V_Left_Group"].rotation_euler = (0.0, math.radians(v_left_degrees), 0.0)


def key_pose(arm_obj, frame, h_bottom_degrees, v_left_degrees):
    bpy.context.scene.frame_set(frame)
    set_pose(arm_obj, h_bottom_degrees, v_left_degrees)
    for sheet_index in range(1, SHEET_COUNT + 1):
        prefix = f"Sheet_{sheet_index:02d}"
        for bone_name in (
            f"{prefix}_Fold_H_Bottom_Right",
            f"{prefix}_Fold_H_Bottom_Left",
            f"{prefix}_Fold_V_Left_Group",
        ):
            arm_obj.pose.bones[bone_name].keyframe_insert(data_path="rotation_euler", frame=frame)


def make_action(arm_obj, action_name, keys, frame_start, frame_end):
    action = bpy.data.actions.new(action_name)
    action.use_fake_user = True
    action.use_frame_range = True
    action.frame_start = frame_start
    action.frame_end = frame_end
    action["UE_ImportIntent"] = "Import as Animation Sequence for SK_Newspaper_3Sheets."

    arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    for frame, h_bottom_degrees, v_left_degrees in keys:
        key_pose(arm_obj, frame, h_bottom_degrees, v_left_degrees)
    return action


def create_actions(arm_obj):
    # Corrected user requirement:
    # The long dimension is FULL_HEIGHT_M = 0.84, so the first fold must halve Y:
    # full open 0.60 x 0.84 -> wide half 0.60 x 0.42.
    # The second fold then halves X:
    # wide half 0.60 x 0.42 -> folded quarter 0.30 x 0.42.

    # Unfold order is the inverse: quarter -> wide half -> full open.
    make_action(
        arm_obj,
        ANIM_DEFAULT_FOLDED,
        [(CLIP_START, 180.0, 180.0), (CLIP_START + 1, 180.0, 180.0)],
        CLIP_START,
        CLIP_START + 1,
    )

    make_action(
        arm_obj,
        ANIM_UNFOLD_01,
        [(CLIP_START, 180.0, 180.0), (CLIP_END, 180.0, 0.0)],
        CLIP_START,
        CLIP_END,
    )

    make_action(
        arm_obj,
        ANIM_UNFOLD_02,
        [(CLIP_START, 180.0, 0.0), (CLIP_END, 0.0, 0.0)],
        CLIP_START,
        CLIP_END,
    )

    # Folding order: first fold the 0.84 long side down to 0.42.
    make_action(
        arm_obj,
        ANIM_FOLD_01,
        [(CLIP_START, 0.0, 0.0), (CLIP_END, 180.0, 0.0)],
        CLIP_START,
        CLIP_END,
    )

    # Then fold the remaining width down to the final quarter size.
    make_action(
        arm_obj,
        ANIM_FOLD_02,
        [(CLIP_START, 180.0, 0.0), (CLIP_END, 180.0, 180.0)],
        CLIP_START,
        CLIP_END,
    )

    make_action(
        arm_obj,
        ANIM_PREVIEW,
        [(FRAME_FOLDED, 180.0, 180.0), (FRAME_FIRST_UNFOLD, 180.0, 0.0), (FRAME_FULL_OPEN, 0.0, 0.0)],
        FRAME_FOLDED,
        FRAME_FULL_OPEN,
    )

    make_action(
        arm_obj,
        ANIM_PREVIEW_FOLD,
        [(FRAME_FOLDED, 0.0, 0.0), (FRAME_FIRST_UNFOLD, 180.0, 0.0), (FRAME_FULL_OPEN, 180.0, 180.0)],
        FRAME_FOLDED,
        FRAME_FULL_OPEN,
    )
    arm_obj.animation_data.action = bpy.data.actions[ANIM_DEFAULT_FOLDED]
    bpy.context.scene.frame_set(FRAME_FOLDED)


def finalize_clean_asset(mesh_obj, arm_obj):
    """Keep the source .blend clean and intentional."""
    scene = bpy.context.scene

    for obj in list(scene.objects):
        if obj not in {mesh_obj, arm_obj}:
            bpy.data.objects.remove(obj, do_unlink=True)

    mesh_obj.name = MESH_NAME
    mesh_obj.data.name = MESH_NAME + "_Mesh"
    arm_obj.name = ARMATURE_NAME
    arm_obj.data.name = ARMATURE_NAME + "_Data"

    for obj in (mesh_obj, arm_obj):
        obj.location = (0.0, 0.0, 0.0)
        obj.rotation_euler = (0.0, 0.0, 0.0)
        obj.scale = (1.0, 1.0, 1.0)

    for action in bpy.data.actions:
        action.use_fake_user = True
    if arm_obj.animation_data and ANIM_DEFAULT_FOLDED in bpy.data.actions:
        arm_obj.animation_data.action = bpy.data.actions[ANIM_DEFAULT_FOLDED]

    for datablock_collection in (
        bpy.data.meshes,
        bpy.data.armatures,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.actions,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.curves,
    ):
        for datablock in list(datablock_collection):
            if datablock.users == 0:
                datablock_collection.remove(datablock)

    scene.frame_start = FRAME_FOLDED
    scene.frame_end = FRAME_FULL_OPEN
    scene.frame_set(FRAME_FOLDED)
    scene["DefaultAction"] = ANIM_DEFAULT_FOLDED
    scene["CleanSource"] = "Only SK_Newspaper_3Sheets and ARM_Newspaper_3Sheets should exist in this scene."



# -----------------------------------------------------------------------------
# Export
# -----------------------------------------------------------------------------


def export_fbx(mesh_obj, arm_obj):
    bpy.ops.object.select_all(action="DESELECT")
    arm_obj.select_set(True)
    mesh_obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj

    bpy.ops.export_scene.fbx(
        filepath=str(OUTPUT_FBX),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        bake_anim=True,
        bake_anim_use_all_actions=True,
        bake_anim_use_nla_strips=False,
        bake_anim_force_startend_keying=True,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        path_mode="COPY",
        embed_textures=True,
        axis_forward="-Z",
        axis_up="Y",
        global_scale=1.0,
        apply_unit_scale=True,
        use_space_transform=True,
        primary_bone_axis="Y",
        secondary_bone_axis="X",
        armature_nodetype="NULL",
    )


def main():
    ensure_dirs()
    clear_scene()
    set_scene()

    image = generate_paper_texture(TEXTURE_BASECOLOR)
    material = make_material(image)
    mesh_obj = create_mesh(material)
    arm_obj = create_armature()
    bind_mesh(mesh_obj, arm_obj)
    create_actions(arm_obj)
    finalize_clean_asset(mesh_obj, arm_obj)

    # Final clean source scene: only the newspaper mesh and armature.
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    export_fbx(mesh_obj, arm_obj)

    print(f"Saved blend: {OUTPUT_BLEND}")
    print(f"Saved texture: {TEXTURE_BASECOLOR}")
    print(f"Exported FBX: {OUTPUT_FBX}")


if __name__ == "__main__":
    main()


