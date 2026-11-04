"""Generate the seated office worker seen in reflections (RulesHorror). Blender 5.1.

The office player (`OfficePawn_BP`, C++ AOfficePawn) has no visible body. These two static meshes are attached
to it as `WorldSpaceRepresentation` primitives: hidden from the player's own view, visible in hardware ray
traced reflections (the dark CRT glass) and in shadows.
    SM_OfficeWorker_Head  attached to the camera (turns with the view): head, hair, eyes, neck.
    SM_OfficeWorker_Body  attached to the pawn root, kept upright: shirt with rolled-down sleeves and a dark tie,
                          hands resting on the thighs, trousers, shoes; seated pose.
Both meshes have their origin at the eye point (the camera), Unreal +X forward, +Z up. Colors match the
first person arms of the test character (Art/Blender/FPArms: Sleeve / Skin).

Shapes are overlapping ellipsoids and capsules fused with a voxel remesh, smoothed and decimated (soft, closed surfaces).

Run (no GUI):
    "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup --python create_office_worker_blend.py
    append  -- --preview <dir>  to render front / side previews (not saved into the .blend).
"""

import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Euler, Matrix, Vector
from mathutils.bvhtree import BVHTree

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

BLEND = HERE / "OfficeWorker.blend"
EXPORT_DIR = HERE / "Exports"

MATERIALS = {
    # name: (linear color, metallic, roughness)
    "Shirt": ((0.55, 0.60, 0.66, 1.0), 0.0, 0.80),     # same as FPArms "Sleeve"
    "Skin": ((0.55, 0.38, 0.30, 1.0), 0.0, 0.55),      # same as FPArms "Skin"
    "Trousers": ((0.045, 0.048, 0.056, 1.0), 0.0, 0.75),
    "Shoes": ((0.012, 0.011, 0.011, 1.0), 0.0, 0.35),
    "Tie": ((0.09, 0.025, 0.03, 1.0), 0.0, 0.55),
    "Hair": ((0.022, 0.017, 0.013, 1.0), 0.0, 0.6),
    "Eye": ((0.01, 0.01, 0.01, 1.0), 0.0, 0.15),
}



def ue(x, y, z):
    """Unreal cm (X forward, Y right, Z up) -> Blender m (Blender Y = -Unreal Y)."""
    return Vector((x / 100.0, -y / 100.0, z / 100.0))


# -----------------------------------------------------------------------------
# Shape groups: overlapping ellipsoids / capsules, fused with a voxel remesh, smoothed, decimated
# -----------------------------------------------------------------------------

class Group:
    def __init__(self, name):
        self.name = name
        self.bm = bmesh.new()


def new_group(name):
    return Group(name)


def ellipsoid(g, center, radii, rot=(0.0, 0.0, 0.0), stiffness=None):
    """center/radii in Unreal cm; rot = Unreal (roll about X, pitch about Y, yaw about Z) degrees."""
    part = bmesh.new()
    bmesh.ops.create_uvsphere(part, u_segments=24, v_segments=16, radius=1.0)
    scale = Matrix.Diagonal((radii[0] / 100.0, radii[1] / 100.0, radii[2] / 100.0, 1.0))
    rot_m = Euler((math.radians(rot[0]), math.radians(-rot[1]), math.radians(-rot[2]))).to_matrix().to_4x4()
    bmesh.ops.transform(part, matrix=Matrix.Translation(ue(*center)) @ rot_m @ scale, verts=part.verts)
    rh.merge_into(g.bm, part)


def capsule(g, a, b, radius, stiffness=None):
    """Capsule from Unreal point a to b (cm) with the given radius (cm)."""
    pa, pb = ue(*a), ue(*b)
    d = pb - pa
    part = bmesh.new()
    bmesh.ops.create_cone(part, cap_ends=True, cap_tris=False, segments=20, radius1=radius / 100.0,
                          radius2=radius / 100.0, depth=d.length)
    orient = Vector((0.0, 0.0, 1.0)).rotation_difference(d.normalized()).to_matrix().to_4x4()
    bmesh.ops.transform(part, matrix=Matrix.Translation((pa + pb) * 0.5) @ orient, verts=part.verts)
    rh.merge_into(g.bm, part)
    for p in (pa, pb):
        cap = bmesh.new()
        bmesh.ops.create_uvsphere(cap, u_segments=20, v_segments=12, radius=radius / 100.0,
                                  matrix=Matrix.Translation(p))
        rh.merge_into(g.bm, cap)


def vcapsule(g, top, bottom, rx, ry, rz_cap):
    """Vertical capsule with an elliptic cross-section (rx forward, ry sideways) and ellipsoid caps (cm)."""
    pt, pb = ue(*top), ue(*bottom)
    part = bmesh.new()
    bmesh.ops.create_cone(part, cap_ends=True, cap_tris=False, segments=32, radius1=1.0, radius2=1.0,
                          depth=(pt - pb).length)
    m = Matrix.Translation((pt + pb) * 0.5) @ Matrix.Diagonal((rx / 100.0, ry / 100.0, 1.0, 1.0))
    bmesh.ops.transform(part, matrix=m, verts=part.verts)
    rh.merge_into(g.bm, part)
    for c in (pt, pb):
        cap = bmesh.new()
        bmesh.ops.create_uvsphere(cap, u_segments=32, v_segments=16, radius=1.0)
        bmesh.ops.transform(cap, matrix=Matrix.Translation(c) @ Matrix.Diagonal((rx / 100.0, ry / 100.0, rz_cap / 100.0, 1.0)),
                            verts=cap.verts)
        rh.merge_into(g.bm, cap)


def group_to_mesh(g, mat_index, target_tris, voxel=0.0045, smooth_repeat=6):
    """Fuse the group's overlapping shells into one surface."""
    mesh = bpy.data.meshes.new("_" + g.name)
    g.bm.to_mesh(mesh)
    g.bm.free()
    o = bpy.data.objects.new("_" + g.name, mesh)
    bpy.context.scene.collection.objects.link(o)
    rem = o.modifiers.new("Remesh", "REMESH")
    rem.mode = "VOXEL"
    rem.voxel_size = voxel
    rem.adaptivity = 0.0
    sm = o.modifiers.new("Smooth", "SMOOTH")
    sm.factor = 0.9
    sm.iterations = smooth_repeat
    dec = o.modifiers.new("Decimate", "DECIMATE")
    dec.ratio = 1.0
    deps = bpy.context.evaluated_depsgraph_get()
    full = sum(len(p.vertices) - 2 for p in o.evaluated_get(deps).data.polygons)
    dec.ratio = min(1.0, target_tris / max(full, 1))
    deps = bpy.context.evaluated_depsgraph_get()
    out = bpy.data.meshes.new_from_object(o.evaluated_get(deps))
    bm = bmesh.new()
    bm.from_mesh(out)
    bpy.data.meshes.remove(out)
    bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.meshes.remove(mesh)
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def uv_sphere(center, radius, mat_index, segments=12, rings=8):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius / 100.0,
                              matrix=Matrix.Translation(ue(*center)))
    for f in bm.faces:
        f.material_index = mat_index
    return bm


# -----------------------------------------------------------------------------
# Head (camera space, eye point at the origin)
# -----------------------------------------------------------------------------

def build_head(mat):
    skin = new_group("HeadSkin")
    ellipsoid(skin, (-5.5, 0.0, 3.5), (10.0, 7.8, 11.0))                 # cranium
    ellipsoid(skin, (-3.8, 0.0, -5.0), (6.6, 6.3, 7.2))                  # jaw
    ellipsoid(skin, (0.2, 0.0, -10.4), (3.0, 4.0, 2.4))                  # chin
    for s in (-1.0, 1.0):
        ellipsoid(skin, (-1.2, s * 4.2, -2.8), (3.8, 3.0, 3.4))          # cheeks
    ellipsoid(skin, (1.3, 0.0, -7.0), (1.6, 2.4, 0.9))                   # lips
    ellipsoid(skin, (3.0, 0.0, -1.6), (1.6, 1.3, 2.8), rot=(0, -15, 0), stiffness=3.0)   # nose bridge
    ellipsoid(skin, (4.4, 0.0, -3.6), (1.5, 1.6, 1.2), stiffness=3.0)    # nose tip
    ellipsoid(skin, (2.4, 0.0, 3.4), (2.0, 6.0, 1.6), stiffness=1.5)     # brow ridge
    for s in (-1.0, 1.0):
        ellipsoid(skin, (-6.5, s * 7.6, -0.5), (1.6, 1.0, 3.0), stiffness=2.5)   # ears
    capsule(skin, (-6.0, 0.0, -8.0), (-8.5, 0.0, -24.0), 5.4)           # neck
    head = group_to_mesh(skin, mat["Skin"], 3200)

    hair = new_group("Hair")
    ellipsoid(hair, (-6.0, 0.0, 8.0), (9.9, 8.6, 8.4))                    # top
    ellipsoid(hair, (-10.5, 0.0, 1.5), (6.0, 8.2, 8.5))                   # back of the head
    for s in (-1.0, 1.0):
        ellipsoid(hair, (-5.0, s * 7.2, 4.0), (5.5, 1.8, 4.0))            # sides above the ears
    rh.merge_into(head, group_to_mesh(hair, mat["Hair"], 1600))

    # Eyes: dark, slightly sunk into the sockets.
    for s in (-1.0, 1.0):
        rh.merge_into(head, uv_sphere((2.1, s * 3.2, 0.2), 1.1, mat["Eye"]))
    return head


# -----------------------------------------------------------------------------
# Body (pawn space, eye point at the origin, upright)
# -----------------------------------------------------------------------------

def build_body(mat):
    shirt = new_group("Shirt")
    vcapsule(shirt, (-8.5, 0.0, -31.0), (-8.8, 0.0, -55.0), 10.5, 16.2, 8.5)   # torso
    ellipsoid(shirt, (-6.0, 0.0, -35.0), (9.0, 13.5, 9.0))                # chest
    for s in (-1.0, 1.0):
        ellipsoid(shirt, (-9.0, s * 8.5, -21.0), (6.0, 8.0, 4.2))         # trapezius
    capsule(shirt, (-9.0, -15.0, -26.0), (-9.0, 15.0, -26.0), 6.5)        # shoulders
    ellipsoid(shirt, (-8.0, 0.0, -21.5), (6.8, 7.4, 3.0), stiffness=1.5)  # collar base
    for s in (-1.0, 1.0):
        capsule(shirt, (-9.0, s * 19.5, -27.0), (-5.0, s * 22.0, -50.0), 4.9)    # upper arm
        capsule(shirt, (-5.0, s * 22.0, -50.0), (18.0, s * 16.0, -61.0), 4.1)    # forearm (sleeve)
    body = group_to_mesh(shirt, mat["Shirt"], 5000, smooth_repeat=16)

    skin = new_group("Hands")
    for s in (-1.0, 1.0):
        ellipsoid(skin, (24.5, s * 14.5, -62.5), (5.2, 2.6, 4.4), rot=(s * 10.0, 10.0, -s * 15.0))  # hand on the thigh
        capsule(skin, (20.0, s * 12.0, -61.0), (25.0, s * 10.0, -60.0), 1.3)                          # thumb
        capsule(skin, (17.0, s * 15.6, -61.5), (20.5, s * 15.0, -62.0), 3.0)                          # wrist
    rh.merge_into(body, group_to_mesh(skin, mat["Skin"], 1200))

    trousers = new_group("Trousers")
    ellipsoid(trousers, (-9.5, 0.0, -67.5), (12.5, 17.0, 9.5))            # hips / seat
    for s in (-1.0, 1.0):
        capsule(trousers, (-7.0, s * 9.0, -70.5), (36.0, s * 11.0, -70.0), 7.6)    # thigh
        capsule(trousers, (37.0, s * 11.0, -70.0), (39.0, s * 12.0, -111.0), 5.6)  # shin
    rh.merge_into(body, group_to_mesh(trousers, mat["Trousers"], 3000))

    shoes = new_group("Shoes")
    for s in (-1.0, 1.0):
        ellipsoid(shoes, (44.0, s * 12.0, -116.5), (10.5, 4.6, 4.0))
        ellipsoid(shoes, (39.0, s * 12.0, -113.5), (5.0, 4.4, 3.6))
    rh.merge_into(body, group_to_mesh(shoes, mat["Shoes"], 800))

    add_tie(body, mat["Tie"], mat["Shirt"])
    return body


def add_tie(bm, tie_mat, shirt_mat):
    """Tie strip laid onto the shirt front (ray cast from the front), plus a knot."""
    shirt_faces = [f for f in bm.faces if f.material_index == shirt_mat]
    tmp = bmesh.new()
    vmap = {}
    for f in shirt_faces:
        vs = []
        for v in f.verts:
            if v not in vmap:
                vmap[v] = tmp.verts.new(v.co)
            vs.append(vmap[v])
        tmp.faces.new(vs)
    tree = BVHTree.FromBMesh(tmp)
    tmp.free()

    def surface_x(y, z):
        origin = ue(20.0, y, z)
        hit = tree.ray_cast(origin, Vector((-1.0, 0.0, 0.0)))
        return hit[0].x * 100.0 if hit[0] is not None else -2.0

    # (z, half width) from knot to tip, resampled every centimeter so the strip follows the chest.
    keys = [(-22.0, 1.6), (-26.0, 2.1), (-34.0, 2.9), (-44.0, 3.6), (-52.0, 4.0), (-55.5, 2.2), (-57.0, 0.2)]
    profile = []
    for (z0, w0), (z1, w1) in zip(keys, keys[1:]):
        n = max(1, int(round(abs(z1 - z0))))
        for k in range(n):
            t = k / n
            profile.append((z0 + (z1 - z0) * t, w0 + (w1 - w0) * t))
    profile.append(keys[-1])
    tie = bmesh.new()
    front, back = [], []
    for z, hw in profile:
        row_f, row_b = [], []
        for y in (-hw, 0.0, hw):
            x = surface_x(y, z) + (0.45 if y == 0.0 else 0.3)
            row_f.append(tie.verts.new(ue(x, y, z)))
            row_b.append(tie.verts.new(ue(x - 0.6, y, z)))
        front.append(row_f)
        back.append(row_b)
    for i in range(len(profile) - 1):
        for j in range(2):
            tie.faces.new((front[i][j], front[i + 1][j], front[i + 1][j + 1], front[i][j + 1]))
            tie.faces.new((back[i][j + 1], back[i + 1][j + 1], back[i + 1][j], back[i][j]))
        for j in (0, 2):
            a, b = (front, back) if j == 0 else (back, front)
            tie.faces.new((a[i][j], a[i + 1][j], b[i + 1][j], b[i][j]))
    tie.faces.new([front[0][0], front[0][1], front[0][2], back[0][2], back[0][1], back[0][0]])
    tie.faces.new([back[-1][0], back[-1][1], back[-1][2], front[-1][2], front[-1][1], front[-1][0]])
    bmesh.ops.recalc_face_normals(tie, faces=tie.faces)
    for f in tie.faces:
        f.material_index = tie_mat
    # Knot.
    kx = surface_x(0.0, -20.5) + 0.3
    knot = rh.box(*(ue(kx - 1.2, 0, 0).x, ue(kx + 1.0, 0, 0).x), -0.015, 0.015, -0.225, -0.19, tie_mat)
    rh.merge_into(tie, knot)
    rh.merge_into(bm, tie)


# -----------------------------------------------------------------------------
# Output
# -----------------------------------------------------------------------------

def finish(bm, name, mats):
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    rh.set_smoothing(bm, 60.0)
    rh.box_project_uvs(bm, 1.0)
    mesh = bpy.data.meshes.new(name + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for m in mats:
        mesh.materials.append(m)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def preview(objs, out_dir):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 900, 900
    cam_data = bpy.data.cameras.new("_PreviewCam")
    cam_data.lens = 50.0
    cam = bpy.data.objects.new("_PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    views = {
        # name: (Unreal cm location, look-at)
        "front": ((150.0, 25.0, -20.0), (0.0, 0.0, -45.0)),
        "side": ((10.0, 170.0, -40.0), (5.0, 0.0, -55.0)),
        "face": ((60.0, 8.0, -3.0), (0.0, 0.0, -6.0)),
        "back": ((-120.0, -60.0, 10.0), (0.0, 0.0, -45.0)),
    }
    for name, (loc, tgt) in views.items():
        a, b = ue(*loc), ue(*tgt)
        cam.location = a
        cam.rotation_euler = (b - a).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(out_dir / f"worker_{name}.png")
        bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.cameras.remove(cam_data)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    rh.clear_scene()
    rh.set_scene()
    mats = {n: rh.make_material(n, c, metallic=m, roughness=r) for n, (c, m, r) in MATERIALS.items()}

    head_slots = ["Skin", "Hair", "Eye"]
    body_slots = ["Shirt", "Skin", "Trousers", "Shoes", "Tie"]
    head = finish(build_head({n: i for i, n in enumerate(head_slots)}), "SM_OfficeWorker_Head",
                  [mats[n] for n in head_slots])
    body = finish(build_body({n: i for i, n in enumerate(body_slots)}), "SM_OfficeWorker_Body",
                  [mats[n] for n in body_slots])
    for o in (head, body):
        rh.report(o)
    if "--preview" in argv:
        preview([head, body], argv[argv.index("--preview") + 1])
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    for o in (head, body):
        rh.export_fbx(o, EXPORT_DIR / f"{o.name}.fbx")
    print(f"Saved {BLEND}")


main()
