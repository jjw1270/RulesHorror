"""Generate the L_Office room shell (walls, floor, ceiling, baseboards) for RulesHorror. Blender 5.1.

Replaces the four BSP box brushes of L_Office (folder `Room`), which used the engine default checker material:
    additive 800 x 620 x 260 box + subtractive 790 x 610 x 250 box (5 cm walls),
    door hole in the +X wall (y -200 .. -104, z up to 68), window hole in the -X wall (y +-223.6, z -10 .. 110).
The interior is kept exactly the same (x +-395, y +-305, z -125 .. 125 cm, Unreal space, origin = room center),
walls/floor/ceiling are 10 cm thick (outward) so Lumen does not leak light.

Run (no GUI):
    "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup --python create_office_room_blend.py
    append  -- --preview <dir>  to render an interior preview (not saved into the .blend).

Material slots: Wall, Floor, Ceiling, Baseboard. UVs: box projection, 1 UV unit = 1 m, origin at the room center
(the carpet / ceiling tile textures are laid out for this, see Art/Textures/OfficeRoom).
"""

import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

NAME = "SM_OfficeRoom"
BLEND = HERE / "OfficeRoom.blend"
FBX = HERE / "Exports" / f"{NAME}.fbx"

WALL, FLOOR, CEILING, BASEBOARD = range(4)

# Interior (Unreal cm)
IX, IY, Z0, Z1 = 395.0, 305.0, -125.0, 125.0
T = 10.0                                   # shell thickness
DOOR_Y0, DOOR_Y1, DOOR_Z1 = -200.0, -104.0, 68.0
WIN_Y, WIN_Z0, WIN_Z1 = 223.6, -10.0, 110.0
BASE_H, BASE_T = 10.0, 1.2                 # baseboard height / thickness
DOOR_SLAB_Y0, DOOR_SLAB_Y1 = -202.0, -101.0  # BP_OfficeDoor slab edges (baseboard stops against it)


def ue_box(x0, x1, y0, y1, z0, z1, mat):
    """Box given in Unreal cm (Blender Y = -Unreal Y)."""
    return rh.box(x0 / 100.0, x1 / 100.0, -y1 / 100.0, -y0 / 100.0, z0 / 100.0, z1 / 100.0, mat)


def set_face_material(bm, normal, mat):
    """Faces whose normal matches `normal` (Blender space) get `mat`."""
    n = Vector(normal)
    for f in bm.faces:
        if f.normal.dot(n) > 0.99:
            f.material_index = mat


def build():
    bm = bmesh.new()
    parts = []

    # Floor slab: only the top face shows (carpet).
    floor = ue_box(-IX - T, IX + T, -IY - T, IY + T, Z0 - T, Z0, WALL)
    floor.normal_update()
    set_face_material(floor, (0, 0, 1), FLOOR)
    parts.append(floor)

    # Ceiling slab: only the bottom face shows (ceiling tiles).
    ceiling = ue_box(-IX - T, IX + T, -IY - T, IY + T, Z1, Z1 + T, WALL)
    ceiling.normal_update()
    set_face_material(ceiling, (0, 0, -1), CEILING)
    parts.append(ceiling)

    # Long walls (+Y / -Y), full length so they close the corners.
    parts.append(ue_box(-IX - T, IX + T, IY, IY + T, Z0, Z1, WALL))
    parts.append(ue_box(-IX - T, IX + T, -IY - T, -IY, Z0, Z1, WALL))

    # +X wall with the door hole.
    parts.append(ue_box(IX, IX + T, -IY, DOOR_Y0, Z0, Z1, WALL))
    parts.append(ue_box(IX, IX + T, DOOR_Y1, IY, Z0, Z1, WALL))
    parts.append(ue_box(IX, IX + T, DOOR_Y0, DOOR_Y1, DOOR_Z1, Z1, WALL))

    # -X wall with the window hole.
    parts.append(ue_box(-IX - T, -IX, -IY, -WIN_Y, Z0, Z1, WALL))
    parts.append(ue_box(-IX - T, -IX, WIN_Y, IY, Z0, Z1, WALL))
    parts.append(ue_box(-IX - T, -IX, -WIN_Y, WIN_Y, Z0, WIN_Z0, WALL))
    parts.append(ue_box(-IX - T, -IX, -WIN_Y, WIN_Y, WIN_Z1, Z1, WALL))

    # Baseboards (rubber cove base) on the room side, with a softened top edge.
    zb0, zb1 = Z0, Z0 + BASE_H
    boards = [
        (-IX, IX, IY - BASE_T, IY),                       # +Y wall
        (-IX, IX, -IY, -IY + BASE_T),                     # -Y wall
        (-IX, -IX + BASE_T, -IY + BASE_T, IY - BASE_T),   # -X wall
        (IX - BASE_T, IX, -IY + BASE_T, DOOR_SLAB_Y0),    # +X wall, left of the door
        (IX - BASE_T, IX, DOOR_SLAB_Y1, IY - BASE_T),     # +X wall, right of the door
    ]
    for x0, x1, y0, y1 in boards:
        b = ue_box(x0, x1, y0, y1, zb0, zb1, BASEBOARD)
        top = [e for e in b.edges if all(abs(v.co.z - zb1 / 100.0) < 1e-6 for v in e.verts)]
        rh.bevel_edges(b, top, 0.003, 2)
        for f in b.faces:
            f.material_index = BASEBOARD
        parts.append(b)

    for p in parts:
        rh.merge_into(bm, p)
    return bm


def finish(bm, materials):
    # Every part is a closed box with outward normals already; no vertex merging (touching boxes would turn
    # into non-manifold geometry and confuse normal recalculation).
    for f in bm.faces:
        f.smooth = False
    bm.normal_update()  # box projection picks the axis from the face normal
    rh.box_project_uvs(bm, 1.0)
    mesh = bpy.data.meshes.new(NAME + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for m in materials:
        mesh.materials.append(m)
    obj = bpy.data.objects.new(NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def preview(obj, out_dir):
    """Interior views with a temporary camera (workbench, textures off)."""
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    cam_data = bpy.data.cameras.new("_PreviewCam")
    cam_data.lens = 16.0
    cam = bpy.data.objects.new("_PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    light_data = bpy.data.lights.new("_PreviewLight", "POINT")
    light_data.energy = 400.0
    light = bpy.data.objects.new("_PreviewLight", light_data)
    light.location = (0.0, 0.0, 1.0)
    scene.collection.objects.link(light)
    scene.camera = cam
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 1024, 576
    views = {
        # name: (Unreal cm location, look-at Unreal cm)
        "door_wall": ((-300.0, 200.0, 40.0), (395.0, -150.0, -20.0)),
        "window_wall": ((300.0, -200.0, 40.0), (-395.0, 0.0, 30.0)),
    }
    for name, (loc, target) in views.items():
        loc_b = Vector((loc[0] / 100.0, -loc[1] / 100.0, loc[2] / 100.0))
        tgt_b = Vector((target[0] / 100.0, -target[1] / 100.0, target[2] / 100.0))
        cam.location = loc_b
        cam.rotation_euler = (tgt_b - loc_b).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(out_dir / f"{NAME}_{name}.png")
        bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.objects.remove(light, do_unlink=True)
    bpy.data.cameras.remove(cam_data)
    bpy.data.lights.remove(light_data)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    rh.clear_scene()
    rh.set_scene()
    mats = [
        rh.make_material("Wall", (0.56, 0.53, 0.45, 1.0), roughness=0.85),
        rh.make_material("Floor", (0.07, 0.08, 0.10, 1.0), roughness=0.95),
        rh.make_material("Ceiling", (0.66, 0.64, 0.58, 1.0), roughness=0.9),
        rh.make_material("Baseboard", (0.03, 0.03, 0.03, 1.0), roughness=0.6),
    ]
    obj = finish(build(), mats)
    rh.report(obj)
    if "--preview" in argv:
        preview(obj, argv[argv.index("--preview") + 1])
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    rh.export_fbx(obj, FBX)
    print(f"Saved {BLEND} and {FBX}")


main()
