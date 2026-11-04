"""Generate the computer cables for the L_Office desk computer (RulesHorror). Blender 5.1.

One static mesh, `SM_ComputerCables`, modeled in the local space of `/Game/Objects/Interactable/Computer_BP`
(root at the desk top under the monitor; the actor sits at L_Office (328, 268, -42) with yaw 90), so the component
is added to the BP at the origin with no rotation. Contents:
    keyboard and mouse cables (beige, PS/2 plugs in the tower back),
    monitor VGA cable (grey, hangs in a loop behind the desk, plug in the graphics card slot),
    monitor and tower power cords (black) down behind the desk to a power strip on the floor,
    power strip cord to a wall outlet.
The floor part assumes the L_Office layout (83 cm desk against the +Y wall, floor 83 cm below the desk top).

Run (no GUI):
    "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup --python create_computer_cables_blend.py
    append  -- --preview <dir>  to load the surrounding meshes, check clearances and render previews
    (the reference meshes are removed again before saving).

Points are written in L_Office world coordinates (Unreal cm) and converted with `W()`.
"""

import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

HERE = Path(__file__).resolve().parent
ART = HERE.parent
sys.path.insert(0, str(ART / "_Common"))
import rh_prop_utils as rh  # noqa: E402

NAME = "SM_ComputerCables"
BLEND = HERE / "ComputerCables.blend"
FBX = HERE / "Exports" / f"{NAME}.fbx"

# Computer_BP actor transform in L_Office.
BP_X, BP_Y, BP_Z = 328.0, 268.0, -42.0
FLOOR_Z, DESK_Z = -125.0, -42.0

# Material slots.
SLOTS = [
    ("Cable_Beige", (0.58, 0.55, 0.47, 1.0), 0.0, 0.55),
    ("Cable_Black", (0.018, 0.018, 0.018, 1.0), 0.0, 0.45),
    ("Cable_Grey", (0.11, 0.11, 0.115, 1.0), 0.0, 0.5),
    ("Plastic_Beige", (0.62, 0.58, 0.48, 1.0), 0.0, 0.55),
    ("Panel_Dark", (0.03, 0.03, 0.03, 1.0), 0.0, 0.6),
    ("Metal", (0.6, 0.6, 0.62, 1.0), 1.0, 0.35),
    ("Switch_Red", (0.6, 0.05, 0.03, 1.0), 0.0, 0.3),
    ("Plastic_OffWhite", (0.70, 0.69, 0.64, 1.0), 0.0, 0.45),
]
I_BEIGE, I_BLACK, I_GREY, I_PBEIGE, I_DARK, I_METAL, I_RED, I_WHITE = range(len(SLOTS))


def W(x, y, z):
    """L_Office world (cm) -> Blender coordinates in Computer_BP local space (m)."""
    return Vector(((y - BP_Y) / 100.0, (x - BP_X) / 100.0, (z - BP_Z) / 100.0))


def wbox(x0, x1, y0, y1, z0, z1, mat, bevel=0.0):
    """Axis-aligned box given in world cm."""
    a, b = W(x0, y0, z0), W(x1, y1, z1)
    xs, ys, zs = sorted((a.x, b.x)), sorted((a.y, b.y)), sorted((a.z, b.z))
    if bevel > 0:
        return rh.beveled_box(xs[0], xs[1], ys[0], ys[1], zs[0], zs[1], bevel / 100.0, mat, 1)
    return rh.box(xs[0], xs[1], ys[0], ys[1], zs[0], zs[1], mat)


def wcyl(radius, length, center, axis, mat, segments=12, radius2=None):
    """Cylinder in world cm. axis: 'X'/'Y'/'Z' in world; world X/Y swap in BP space."""
    blender_axis = {"X": "Y", "Y": "X", "Z": "Z"}[axis]
    return rh.cylinder(radius / 100.0, length / 100.0, W(*center), blender_axis, mat, segments,
                       None if radius2 is None else radius2 / 100.0)


# -----------------------------------------------------------------------------
# Tube along a smooth path
# -----------------------------------------------------------------------------

def catmull_rom(points, step):
    """Centripetal Catmull-Rom through `points` (Vectors), sampled about every `step` meters."""
    pts = [points[0] + (points[0] - points[1])] + list(points) + [points[-1] + (points[-1] - points[-2])]
    out = []
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = pts[i - 1], pts[i], pts[i + 1], pts[i + 2]

        def tj(ti, a, b):
            return ti + max((b - a).length, 1e-6) ** 0.5

        t0 = 0.0
        t1 = tj(t0, p0, p1)
        t2 = tj(t1, p1, p2)
        t3 = tj(t2, p2, p3)
        n = max(2, int(math.ceil((p2 - p1).length / step)))
        for k in range(n):
            t = t1 + (t2 - t1) * k / n
            a1 = p0 * ((t1 - t) / (t1 - t0)) + p1 * ((t - t0) / (t1 - t0))
            a2 = p1 * ((t2 - t) / (t2 - t1)) + p2 * ((t - t1) / (t2 - t1))
            a3 = p2 * ((t3 - t) / (t3 - t2)) + p3 * ((t - t2) / (t3 - t2))
            b1 = a1 * ((t2 - t) / (t2 - t0)) + a2 * ((t - t0) / (t2 - t0))
            b2 = a2 * ((t3 - t) / (t3 - t1)) + a3 * ((t - t1) / (t3 - t1))
            out.append(b1 * ((t2 - t) / (t2 - t1)) + b2 * ((t - t1) / (t2 - t1)))
    out.append(points[-1].copy())
    return out


def tube(path, radius, mat, sides=8):
    """Closed tube (capped) with rotation-minimizing frames."""
    bm = bmesh.new()
    tangents = []
    for i in range(len(path)):
        a = path[max(i - 1, 0)]
        b = path[min(i + 1, len(path) - 1)]
        tangents.append((b - a).normalized())
    up = Vector((0, 0, 1)) if abs(tangents[0].z) < 0.9 else Vector((1, 0, 0))
    normal = tangents[0].cross(up).normalized()
    rings = []
    for i, p in enumerate(path):
        t = tangents[i]
        if i > 0:
            # parallel transport of the previous normal
            normal = (normal - t * normal.dot(t)).normalized()
        binormal = t.cross(normal)
        ring = []
        for s in range(sides):
            ang = 2.0 * math.pi * s / sides
            ring.append(bm.verts.new(p + (normal * math.cos(ang) + binormal * math.sin(ang)) * radius))
        rings.append(ring)
    for i in range(len(rings) - 1):
        r0, r1 = rings[i], rings[i + 1]
        for s in range(sides):
            f = bm.faces.new((r0[s], r0[(s + 1) % sides], r1[(s + 1) % sides], r1[s]))
            f.material_index = mat
            f.smooth = True
    cap0 = bm.faces.new(list(reversed(rings[0])))
    cap1 = bm.faces.new(rings[-1])
    for f in (cap0, cap1):
        f.material_index = mat
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def cable(world_points, radius_cm, mat):
    pts = [W(*p) for p in world_points]
    return tube(catmull_rom(pts, 0.008), radius_cm / 100.0, mat)


# -----------------------------------------------------------------------------
# Layout (L_Office world cm)
# -----------------------------------------------------------------------------
# Tower back face y = 294.8, x 359.8 .. 380.5; desk back edge y = 299.1 at z -42; wall y = 305.
# Monitor back connector blocks at about (321.5, 290.4, -27) and (336, 290.4, -27).

TOWER_BACK_Y = 294.8
PS2_KB = (377.0, -12.0)     # (x, z) keyboard port
PS2_MS = (377.0, -9.6)      # mouse port (stacked above)
VGA = (374.0, -30.0)        # graphics card slot
PSU = (366.0, -3.5)         # power inlet
STRIP_X0, STRIP_X1, STRIP_Y, STRIP_Z1 = 314.0, 348.0, 294.0, -121.0
OUTLET = (300.0, -100.0)    # wall outlet on the +Y wall (x, z)

DESK_TOP = -42.05                          # desk top surface (world z)


def on_desk(r):
    """Center height of a cable of radius r (cm) lying on the desk."""
    return DESK_TOP + r + 0.1


KB_Z, MS_Z = on_desk(0.25), on_desk(0.22)
KEYBOARD_CABLE = [
    (352.0, 228.3, -41.0), (353.5, 233.0, KB_Z), (355.5, 245.0, KB_Z), (356.6, 265.0, KB_Z),
    (357.0, 285.0, KB_Z), (358.6, 294.6, KB_Z), (362.0, 297.4, KB_Z), (367.0, 297.7, KB_Z),
    (371.0, 297.8, KB_Z + 0.5), (373.8, 297.8, -37.5), (375.8, 297.6, -27.0), (376.8, 297.3, -17.0),
    (377.0, TOWER_BACK_Y + 2.6, PS2_KB[1] - 0.8), (377.0, TOWER_BACK_Y + 2.1, PS2_KB[1]),
]
MOUSE_CABLE = [
    (293.4, 228.7, -40.75), (293.8, 231.6, -40.62), (294.4, 234.4, -41.2), (295.4, 238.5, MS_Z), (296.8, 248.0, MS_Z), (301.0, 266.0, MS_Z),
    (303.0, 284.0, MS_Z), (306.5, 295.5, MS_Z), (312.0, 298.3, MS_Z), (330.0, 298.6, MS_Z),
    (352.0, 298.6, MS_Z), (367.0, 298.6, MS_Z), (371.5, 298.7, MS_Z + 0.5), (374.5, 298.7, -37.0),
    (376.3, 298.4, -26.0), (376.9, 297.9, -15.0),
    (377.0, TOWER_BACK_Y + 2.7, PS2_MS[1] - 0.8), (377.0, TOWER_BACK_Y + 2.1, PS2_MS[1]),
]
VGA_CABLE = [
    (336.0, 290.0, -27.0), (336.2, 293.0, -27.6), (337.0, 297.0, -32.5), (338.0, 299.9, -41.2),
    (339.5, 301.6, -47.0), (345.0, 302.3, -62.0), (354.0, 302.4, -67.0), (363.0, 302.3, -58.0),
    (369.5, 301.5, -44.0), (372.5, 300.1, -36.0), (374.0, TOWER_BACK_Y + 3.6, VGA[1] - 1.0),
    (374.0, TOWER_BACK_Y + 2.6, VGA[1]),
]
MONITOR_POWER = [
    (321.5, 290.0, -27.0), (321.6, 293.0, -27.6), (322.0, 297.2, -33.0), (322.4, 299.9, -41.2),
    (322.8, 301.7, -47.0), (323.2, 302.3, -70.0), (323.4, 302.3, -100.0), (324.0, 301.0, -117.0),
    (325.6, 297.6, -120.4), (327.0, STRIP_Y, STRIP_Z1 + 1.5),
]
TOWER_POWER = [
    (PSU[0], TOWER_BACK_Y + 2.6, PSU[1]), (PSU[0], TOWER_BACK_Y + 4.0, PSU[1] - 1.2), (368.5, 301.0, -8.0),
    (374.0, 302.1, -13.0), (378.0, 302.4, -24.0), (378.6, 302.4, -60.0), (378.2, 302.4, -100.0),
    (374.0, 301.8, -117.0), (365.0, 300.2, -123.6), (350.0, 298.8, -124.0), (341.0, STRIP_Y + 3.0, -123.2),
    (336.0, STRIP_Y + 1.0, STRIP_Z1 + 1.5),
]
STRIP_CORD = [
    (STRIP_X0 - 0.5, STRIP_Y, -123.0), (310.0, STRIP_Y + 1.0, -124.3), (305.0, 298.5, -124.4),
    (302.0, 301.5, -123.0), (300.5, 302.6, -116.0), (300.0, 302.7, -108.0), (300.0, 302.6, OUTLET[1] - 3.2),
]


def plugs_and_parts():
    parts = []
    # PS/2 plugs (round, beige boot) pointing into the tower back (-Y).
    for (x, z) in (PS2_KB, PS2_MS):
        parts.append(wcyl(0.62, 1.7, (x, TOWER_BACK_Y + 0.85, z), "Y", I_PBEIGE, 12))
        parts.append(wcyl(0.42, 0.6, (x, TOWER_BACK_Y + 1.95, z), "Y", I_PBEIGE, 10, 0.32))
    # VGA plug: dark shell with two thumb screws.
    vx, vz = VGA
    parts.append(wbox(vx - 2.0, vx + 2.0, TOWER_BACK_Y, TOWER_BACK_Y + 1.6, vz - 0.8, vz + 0.8, I_DARK, 0.15))
    parts.append(wbox(vx - 1.1, vx + 1.1, TOWER_BACK_Y + 1.6, TOWER_BACK_Y + 2.8, vz - 0.65, vz + 0.65, I_DARK, 0.2))
    for dx in (-1.6, 1.6):
        parts.append(wcyl(0.32, 1.4, (vx + dx, TOWER_BACK_Y + 1.6 + 0.7, vz), "Y", I_METAL, 10))
    # Power inlet plug (IEC, black).
    px, pz = PSU
    parts.append(wbox(px - 1.3, px + 1.3, TOWER_BACK_Y, TOWER_BACK_Y + 2.6, pz - 0.9, pz + 0.9, I_BLACK, 0.25))
    # Strain reliefs where the monitor cables leave the rear shell.
    for x in (321.5, 336.0):
        parts.append(wcyl(0.75, 1.0, (x, 290.3, -27.0), "Y", I_DARK, 10, 0.55))

    # Power strip on the floor behind the desk: beige body, 4 dark sockets, lit red rocker switch.
    y0, y1 = STRIP_Y - 2.6, STRIP_Y + 2.6
    parts.append(wbox(STRIP_X0, STRIP_X1, y0, y1, FLOOR_Z, STRIP_Z1, I_PBEIGE, 0.6))
    for sx in (320.0, 327.0, 334.0, 341.0):
        parts.append(wbox(sx - 1.6, sx + 1.6, STRIP_Y - 1.6, STRIP_Y + 1.6, STRIP_Z1 - 0.2, STRIP_Z1 + 0.05, I_DARK))
    parts.append(wbox(STRIP_X1 - 4.2, STRIP_X1 - 1.4, STRIP_Y - 1.3, STRIP_Y + 1.3, STRIP_Z1 - 0.2, STRIP_Z1 + 0.6, I_RED, 0.2))
    # Plugs sitting in sockets 2 and 3 (monitor, tower) with their boots.
    for sx in (327.0, 334.0 + 2.0):
        parts.append(wbox(sx - 1.4, sx + 1.4, STRIP_Y - 1.7, STRIP_Y + 1.7, STRIP_Z1, STRIP_Z1 + 2.2, I_BLACK, 0.3))

    # Wall outlet plate (+Y wall, faces -Y) with the strip plug in the lower socket.
    ox, oz = OUTLET
    parts.append(wbox(ox - 4.0, ox + 4.0, 304.6, 305.0, oz - 6.0, oz + 6.0, I_WHITE, 0.15))
    for dz in (-3.0, 3.0):
        parts.append(wbox(ox - 1.6, ox + 1.6, 304.45, 304.6, oz + dz - 1.6, oz + dz + 1.6, I_WHITE, 0.05))
        parts.append(wbox(ox - 0.9, ox + 0.9, 304.42, 304.46, oz + dz - 0.9, oz + dz + 0.9, I_DARK))
    parts.append(wbox(ox - 1.5, ox + 1.5, 302.2, 304.45, oz - 3.0 - 1.4, oz - 3.0 + 1.4, I_BLACK, 0.3))
    return parts


def build():
    bm = bmesh.new()
    parts = [
        cable(KEYBOARD_CABLE, 0.25, I_BEIGE),
        cable(MOUSE_CABLE, 0.22, I_BEIGE),
        cable(VGA_CABLE, 0.42, I_GREY),
        cable(MONITOR_POWER, 0.36, I_BLACK),
        cable(TOWER_POWER, 0.36, I_BLACK),
        cable(STRIP_CORD, 0.36, I_BLACK),
    ] + plugs_and_parts()
    for p in parts:
        rh.merge_into(bm, p)
    return bm


def finish(bm, mats):
    rh.box_project_uvs(bm, 1.0)
    mesh = bpy.data.meshes.new(NAME + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for m in mats:
        mesh.materials.append(m)
    obj = bpy.data.objects.new(NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for p in mesh.polygons:
        p.use_smooth = p.material_index in (I_BEIGE, I_BLACK, I_GREY) and len(p.vertices) == 4
    return obj


# -----------------------------------------------------------------------------
# Preview: reference meshes in BP space, clearance check, renders
# -----------------------------------------------------------------------------

# (fbx, Unreal BP-local location cm, yaw deg)
REFERENCE = [
    (ART / "ComputerMonitor/Exports/SM_ComputerMonitor.fbx", (0.0, 0.0, 0.0), 90.0),
    (ART / "RetroComputer/Exports/SM_PCTower.fbx", (0.0, -42.0, 0.0), 90.0),
    (ART / "RetroComputer/Exports/SM_KeyboardStandard.fbx", (-54.2, -11.1, 0.0), 90.0),
    (ART / "RetroComputer/Exports/SM_Mouse.fbx", (-45.4, 35.1, 0.0), 85.0),
    (ART / "RetroComputer/Exports/SM_MousePad.fbx", (-46.0, 47.0, 0.0), -90.0),
    (ART / "OfficeDesk/Exports/SM_OfficeDesk_A.fbx", (32.0, 33.0, -83.0), 90.0),
    (ART / "OfficeRoom/Exports/SM_OfficeRoom.fbx", (-268.0, 328.0, 42.0), -90.0),
]


def load_reference():
    objs = []
    for path, loc, yaw in REFERENCE:
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(path))
        for o in set(bpy.data.objects) - before:
            if o.type != "MESH":
                continue
            o.location = (loc[0] / 100.0, -loc[1] / 100.0, loc[2] / 100.0)
            o.rotation_euler = (0.0, 0.0, math.radians(-yaw))
            o.scale = (1.0, 1.0, 1.0)
            objs.append(o)
    bpy.context.view_layer.update()
    return objs


def clearance(cable_sets, refs):
    """Smallest distance from each cable centerline to the reference meshes (minus the cable radius)."""
    trees = []
    for o in refs:
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bm.transform(o.matrix_world)
        trees.append((o.name, BVHTree.FromBMesh(bm)))
        bm.free()
    report = {}
    for name, pts, radius, skip_ends in cable_sets:
        path = catmull_rom([W(*p) for p in pts], 0.008)
        inner = path[skip_ends:len(path) - skip_ends]
        worst = (1e9, None, None)
        for p in inner:
            for tname, tree in trees:
                hit = tree.find_nearest(p)
                if hit[0] is not None and hit[3] - radius / 100.0 < worst[0]:
                    worst = (hit[3] - radius / 100.0, tname, tuple(round(c * 100.0, 1) for c in p))
        report[name] = (round(worst[0] * 100.0, 2), worst[1], worst[2])
    # cable vs cable
    paths = {name: (catmull_rom([W(*p) for p in pts], 0.008), radius, skip) for name, pts, radius, skip in cable_sets}
    names = list(paths)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            pa, ra, sa = paths[names[i]]
            pb, rb, sb = paths[names[j]]
            best = 1e9
            for a in pa[sa:len(pa) - sa]:
                for b in pb[sb:len(pb) - sb:2]:
                    d = (a - b).length - (ra + rb) / 100.0
                    if d < best:
                        best = d
            report[names[i] + "/" + names[j]] = (round(best * 100.0, 2), "cable", None)
    return report


def render_views(out_dir, cables_obj, refs):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = True
    scene.render.resolution_x, scene.render.resolution_y = 1280, 720
    cam_data = bpy.data.cameras.new("_PreviewCam")
    cam = bpy.data.objects.new("_PreviewCam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    views = {
        # name: (world location, world look-at, lens mm, ortho scale or 0)
        "seat": ((326.0, 167.0, -4.0), (332.0, 275.0, -40.0), 20.0, 0.0),
        "top": ((338.0, 262.0, 100.0), (338.0, 262.1, -42.0), 50.0, 1.2),
        "behind": ((340.0, 304.0, 20.0), (345.0, 290.0, -60.0), 14.0, 0.0),
        "under": ((330.0, 215.0, -105.0), (335.0, 300.0, -112.0), 16.0, 0.0),
    }
    for name, (loc, tgt, lens, ortho) in views.items():
        cam_data.lens = lens
        cam_data.type = "ORTHO" if ortho > 0 else "PERSP"
        cam_data.ortho_scale = max(ortho, 0.01)
        cam_data.clip_start = 0.005
        a, b = W(*loc), W(*tgt)
        cam.location = a
        cam.rotation_euler = (b - a).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(out_dir / f"cables_{name}.png")
        bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.cameras.remove(cam_data)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    rh.clear_scene()
    rh.set_scene()
    mats = [rh.make_material(n, c, metallic=m, roughness=r) for n, c, m, r in SLOTS]
    obj = finish(build(), mats)
    rh.report(obj)
    if "--preview" in argv:
        refs = load_reference()
        sets = [("keyboard", KEYBOARD_CABLE, 0.25, 6), ("mouse", MOUSE_CABLE, 0.22, 6),
                ("vga", VGA_CABLE, 0.42, 5), ("monitor_power", MONITOR_POWER, 0.36, 5),
                ("tower_power", TOWER_POWER, 0.36, 5), ("strip_cord", STRIP_CORD, 0.36, 4)]
        for k, v in clearance(sets, refs).items():
            print(f"CLEARANCE {k}: {v[0]} cm to {v[1]} at {v[2]}")
        render_views(argv[argv.index("--preview") + 1], obj, refs)
        for o in refs:
            bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    rh.export_fbx(obj, FBX)
    print(f"Saved {BLEND} and {FBX}")


main()
