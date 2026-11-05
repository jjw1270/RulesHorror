"""Generate the desk computer of L_Office as ONE static mesh (RulesHorror). Blender 5.1.

`SM_Computer` = CRT monitor + tower + keyboard + mouse + all cables (keyboard, mouse, VGA, power cords, power strip on
the floor, wall outlet). It is the mesh of the `Monitor` component of `/Game/Objects/Interactable/Computer_BP`, so it is
modeled in the monitor's own frame (= SM_ComputerMonitor: pivot under the stand, screen faces Unreal +Y). The
ScreenWidget and InteractBox components are attached to `Monitor` and stay exact. Material slot 1 MUST be `Screen`
(C++ AComputer does SetMaterial(1, screen MID)).

The parts are appended from their own sources (ComputerMonitor.blend, RetroComputer.blend) and placed like the
computer in L_Office (keyboard centred in front of the monitor, mouse on the level's mouse pad). The cables are
routed between the parts here, so they always meet the devices.

Run (no GUI):
    "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup --python create_computer_blend.py
    append  -- --preview <dir>  to load the desk, mouse pad and room, check cable clearances and render previews
    (the reference meshes are removed again before saving).

Coordinates:
    * Parts: Computer_BP local space (Unreal cm, yaw in degrees), relative to the BP root.
    * Cable points: L_Office world coordinates of a "design frame" that puts the actor at (328, 268, -42), converted
      with `W()`. The real actor is at (328.44, 267.67, -41.84) (yaw 90), a few mm off, so the level meshes (desk,
      mouse pad, wall) are placed from their real positions relative to the actor (`level_to_design()`).
    * The finished mesh is rotated from BP space into the Monitor component frame (Monitor has yaw 90 in the BP).
"""

import math
import re
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

NAME = "SM_Computer"
BLEND = HERE / "Computer.blend"
FBX = HERE / "Exports" / f"{NAME}.fbx"

# Design frame of the cable points (see the docstring) and the real Computer_BP actor in L_Office.
BP_X, BP_Y, BP_Z = 328.0, 268.0, -42.0
ACTOR = (328.443, 267.672, -41.843)  # yaw 90


def level_to_design(x, y, z):
    """Real L_Office world (cm) -> design frame world (cm), keeping the position relative to the actor."""
    return (x - ACTOR[0] + BP_X, y - ACTOR[1] + BP_Y, z - ACTOR[2] + BP_Z)


def level_to_bp(x, y, z, yaw):
    """Real L_Office world (cm, yaw) -> Computer_BP local (cm, yaw)."""
    return (y - ACTOR[1], -(x - ACTOR[0]), z - ACTOR[2]), yaw - 90.0


FLOOR_Z = level_to_design(0, 0, -125.0)[2]
DESK_TOP = level_to_design(0, 0, -42.05)[2]     # SM_OfficeDesk_A top (desk actor at z -125)
WALL_Y = level_to_design(0, 305.0, 0)[1]        # +Y wall of the room
PAD_TOP_BP = level_to_bp(0, 0, -41.0, 0)[0][2]  # top of the 1 cm mouse pad (pad actor at z -42)

# -----------------------------------------------------------------------------
# Material slots (final order). The monitor's three slots come first so `Screen` is index 1.
# -----------------------------------------------------------------------------
SLOT_ORDER = ["Monitor_Plastic", "Screen", "Monitor_Dark", "Plastic_Beige", "Panel_Dark", "LED", "Keys_Light",
              "Keys_Dark", "Plastic_OffWhite", "Seam_Dark", "Cable_Beige", "Cable_Black", "Cable_Grey", "Switch_Red",
              "Metal"]

# Parts: (source .blend, object, Computer_BP local location cm, yaw deg, source slot -> final slot).
# The BP defaults used to be keyboard (-54.175, -11.091, 0) and mouse (-45.399, 35.106, 0); L_Office overrode them.
PARTS = [
    ("ComputerMonitor/ComputerMonitor.blend", "SM_ComputerMonitor", (0.0, 0.0, 0.0), 90.0,
     {"Plastic_Beige": "Monitor_Plastic", "Plastic_Dark": "Monitor_Dark"}),
    ("RetroComputer/RetroComputer.blend", "SM_PCTower", (0.0, -42.0, 0.0), 90.0, {}),
    ("RetroComputer/RetroComputer.blend", "SM_KeyboardStandard", (-53.175, -0.091, 0.0), 90.0, {}),
    ("RetroComputer/RetroComputer.blend", "SM_Mouse", (-43.399, 44.106, PAD_TOP_BP), 85.0, {}),
]

# Cable mesh slots: name, linear RGBA, metallic, roughness (preview colours; Unreal uses the material instances).
CABLE_SLOTS = [
    ("Cable_Beige", (0.58, 0.55, 0.47, 1.0), 0.0, 0.55),
    ("Cable_Black", (0.018, 0.018, 0.018, 1.0), 0.0, 0.45),
    ("Cable_Grey", (0.11, 0.11, 0.115, 1.0), 0.0, 0.5),
    ("Plastic_Beige", (0.62, 0.58, 0.48, 1.0), 0.0, 0.55),
    ("Panel_Dark", (0.03, 0.03, 0.03, 1.0), 0.0, 0.6),
    ("Metal", (0.6, 0.6, 0.62, 1.0), 1.0, 0.35),
    ("Switch_Red", (0.6, 0.05, 0.03, 1.0), 0.0, 0.3),
    ("Plastic_OffWhite", (0.70, 0.69, 0.64, 1.0), 0.0, 0.45),
]
I_BEIGE, I_BLACK, I_GREY, I_PBEIGE, I_DARK, I_METAL, I_RED, I_WHITE = range(len(CABLE_SLOTS))


def W(x, y, z):
    """Design frame world (cm) -> Blender coordinates in Computer_BP local space (m)."""
    return Vector(((y - BP_Y) / 100.0, (x - BP_X) / 100.0, (z - BP_Z) / 100.0))


def part_point(name, local):
    """Point in a part's own frame (Unreal cm) -> design frame world (cm)."""
    loc, yaw = next((p[2], p[3]) for p in PARTS if p[1] == name)
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    bx = loc[0] + local[0] * c - local[1] * s
    by = loc[1] + local[0] * s + local[1] * c
    return (BP_X - by, BP_Y + bx, BP_Z + loc[2] + local[2])


def kb(x, y, z):  # keyboard frame: back face at y -14.2, bottom z -0.2
    return part_point("SM_KeyboardStandard", (x, y, z))


def ms(x, y, z):  # mouse frame: nose (cable side) toward -y, at y -5.0 for z 1.25, bottom z 0
    return part_point("SM_Mouse", (x, y, z))


def bp_matrix(loc, yaw):
    """Computer_BP local placement (Unreal cm, yaw deg) -> Blender matrix in BP space."""
    return Matrix.Translation((loc[0] / 100.0, -loc[1] / 100.0, loc[2] / 100.0)) @ Matrix.Rotation(math.radians(-yaw), 4, "Z")


# Monitor component: yaw 90 under the BP root. BP space -> Monitor frame is the inverse.
BP_TO_MONITOR = bp_matrix((0.0, 0.0, 0.0), 90.0).inverted()


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
# Cable layout (design frame world cm)
# -----------------------------------------------------------------------------
# Tower back face y = 294.8, x 359.8 .. 380.5. Monitor body x 305.3 .. 350.7, y 245.6 .. 290.4.
# Monitor back connector blocks at about (321.5, 290.4, -27) and (336, 290.4, -27).
# Keyboard: centre (328.1, 214.8), back face y 229.0; the cable starts inside it 12.9 cm toward the tower side.
# Mouse: centre (283.9, 224.6) on the mouse pad (x 266.4 .. 294.8, back edge y 233.5, top z -41.16); the cable
#        starts inside its nose, 1.2 cm above the pad.

TOWER_BACK_Y = 294.8
PS2_KB = (377.0, -12.0)     # (x, z) keyboard port
PS2_MS = (377.0, -9.6)      # mouse port (stacked above)
VGA = (374.0, -30.0)        # graphics card slot
PSU = (366.0, -3.5)         # power inlet
STRIP_X0, STRIP_X1, STRIP_Y, STRIP_Z1 = 314.0, 348.0, 294.0, -121.0
OUTLET = (300.0, -100.0)    # wall outlet on the +Y wall (x, z)
PAD_TOP = PAD_TOP_BP + BP_Z  # mouse pad top in the design frame


def on_desk(r):
    """Center height of a cable of radius r (cm) lying on the desk."""
    return DESK_TOP + r + 0.1


def on_pad(r):
    return PAD_TOP + r + 0.05


KB_Z, MS_Z = on_desk(0.25), on_desk(0.22)
KEYBOARD_CABLE = [
    kb(-12.9, -13.6, 1.0), kb(-12.9, -15.0, 0.95), (341.7, 233.0, KB_Z), (343.8, 238.0, KB_Z), (348.5, 242.8, KB_Z), (353.2, 247.5, KB_Z),
    (355.6, 254.0, KB_Z), (356.6, 265.0, KB_Z),
    (357.0, 285.0, KB_Z), (358.6, 294.6, KB_Z), (362.0, 297.4, KB_Z), (367.0, 297.7, KB_Z),
    (371.0, 297.8, KB_Z + 0.5), (373.8, 297.8, -37.5), (375.8, 297.6, -27.0), (376.8, 297.3, -17.0),
    (377.0, TOWER_BACK_Y + 2.6, PS2_KB[1] - 0.8), (377.0, TOWER_BACK_Y + 2.1, PS2_KB[1]),
]
MOUSE_CABLE = [
    ms(0.0, -4.4, 1.2), ms(0.0, -6.0, 1.15), ms(0.0, -7.2, 0.8), ms(0.0, -8.9, on_pad(0.22) - PAD_TOP),
    (285.3, 234.6, PAD_TOP - 0.2), (285.7, 236.0, MS_Z + 0.35), (286.4, 238.4, MS_Z),
    (289.5, 244.5, MS_Z), (294.5, 252.5, MS_Z), (299.0, 262.0, MS_Z), (301.6, 272.0, MS_Z),
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
    (302.0, 301.5, -123.0), (300.5, 302.6, -116.0), (300.0, 302.7, -108.0), (300.0, WALL_Y - 2.4, OUTLET[1] - 3.2),
]
CABLE_SETS = [  # name, points, radius cm, samples skipped at each end in the clearance check
    ("keyboard", KEYBOARD_CABLE, 0.25, 6), ("mouse", MOUSE_CABLE, 0.22, 3), ("vga", VGA_CABLE, 0.42, 5),
    ("monitor_power", MONITOR_POWER, 0.36, 5), ("tower_power", TOWER_POWER, 0.36, 5), ("strip_cord", STRIP_CORD, 0.36, 4),
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
    wy = WALL_Y
    parts.append(wbox(ox - 4.0, ox + 4.0, wy - 0.4, wy, oz - 6.0, oz + 6.0, I_WHITE, 0.15))
    for dz in (-3.0, 3.0):
        parts.append(wbox(ox - 1.6, ox + 1.6, wy - 0.55, wy - 0.4, oz + dz - 1.6, oz + dz + 1.6, I_WHITE, 0.05))
        parts.append(wbox(ox - 0.9, ox + 0.9, wy - 0.58, wy - 0.54, oz + dz - 0.9, oz + dz + 0.9, I_DARK))
    parts.append(wbox(ox - 1.5, ox + 1.5, wy - 2.8, wy - 0.55, oz - 3.0 - 1.4, oz - 3.0 + 1.4, I_BLACK, 0.3))
    return parts


def build_cables():
    """All cables, plugs, power strip and outlet as one object in BP space."""
    mats = [rh.make_material(n, c, metallic=m, roughness=r) for n, c, m, r in CABLE_SLOTS]
    bm = bmesh.new()
    parts = [cable(pts, radius, mat) for (_, pts, radius, _), mat in
             zip(CABLE_SETS, (I_BEIGE, I_BEIGE, I_GREY, I_BLACK, I_BLACK, I_BLACK))] + plugs_and_parts()
    for p in parts:
        rh.merge_into(bm, p)
    bm.normal_update()
    rh.box_project_uvs(bm, 1.0)
    mesh = bpy.data.meshes.new("Cables_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    for m in mats:
        mesh.materials.append(m)
    for p in mesh.polygons:
        p.use_smooth = p.material_index in (I_BEIGE, I_BLACK, I_GREY) and len(p.vertices) == 4
    obj = bpy.data.objects.new("Cables", mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


# -----------------------------------------------------------------------------
# Parts, merge, collision
# -----------------------------------------------------------------------------

SRC_PREFIX = "_src_"


def base_name(name):
    """Source slot name without Blender's .001 suffix or the prefix final_materials() gives the sources."""
    return re.sub(r"\.\d{3}$", "", name.removeprefix(SRC_PREFIX))


def append_part(blend, name):
    with bpy.data.libraries.load(str(ART / blend), link=False) as (src, dst):
        assert name in src.objects, f"{name} not in {blend}"
        dst.objects = [name]
    obj = dst.objects[0]
    bpy.context.scene.collection.objects.link(obj)
    err = max(abs(a - b) for ra, rb in zip(obj.matrix_world, Matrix.Identity(4)) for a, b in zip(ra, rb))
    assert err < 1e-6, f"{name} is not at the origin"
    return obj


def place_parts():
    parts = []
    for blend, name, loc, yaw, _ in PARTS:
        obj = append_part(blend, name)
        obj.data.transform(bp_matrix(loc, yaw))
        obj.data.update()
        parts.append(obj)
    return parts


def remap_materials(obj, mapping, final_mats):
    """Point the object's faces at the final slot list (same list on every object, so join keeps indices)."""
    me = obj.data
    lookup = [SLOT_ORDER.index(mapping.get(base_name(m.name), base_name(m.name))) for m in me.materials]
    idx = [0] * len(me.polygons)
    me.polygons.foreach_get("material_index", idx)
    idx = [lookup[i] for i in idx]
    me.materials.clear()
    for m in final_mats:
        me.materials.append(m)
    me.polygons.foreach_set("material_index", idx)


def final_materials(objs_and_maps):
    """One material per final slot (copied from the first source material that maps to it, for the preview)."""
    found = {}
    for obj, mapping in objs_and_maps:
        for m in obj.data.materials:
            found.setdefault(mapping.get(base_name(m.name), base_name(m.name)), m)
    missing = [s for s in SLOT_ORDER if s not in found]
    assert not missing, f"no source material for {missing}"
    for m in bpy.data.materials:  # free the plain names: FBX slot names = material names
        m.name = SRC_PREFIX + m.name
    mats = []
    for slot in SLOT_ORDER:
        mat = found[slot].copy()
        mat.name = slot
        assert mat.name == slot, mat.name
        mats.append(mat)
    return mats


def support_hull(obj, matrix, name):
    """Convex collision hull (UCX_) through the extreme vertices of `obj` in many directions (<= 62 points)."""
    pts = [matrix @ v.co for v in obj.data.vertices]
    dirs = []
    ico = bmesh.new()
    bmesh.ops.create_icosphere(ico, subdivisions=2, radius=1.0)
    dirs = [v.co.normalized() for v in ico.verts]
    ico.free()
    dirs += [Vector(d) for d in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))]
    keep = {max(range(len(pts)), key=lambda i: pts[i].dot(d)) for d in dirs}
    bm = bmesh.new()
    for i in keep:
        bm.verts.new(pts[i])
    bmesh.ops.convex_hull(bm, input=bm.verts[:])
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name + "_Mesh")
    bm.to_mesh(mesh)
    bm.free()
    hull = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(hull)
    return hull


def join(objs, name):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    obj.data.name = name + "_Mesh"
    return obj


def sort_faces(obj):
    """Store faces sorted by material index so Unreal's slot order = first-use order = SLOT_ORDER."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.faces.sort(key=lambda f: f.material_index)
    bm.to_mesh(obj.data)
    bm.free()
    idx = [0] * len(obj.data.polygons)
    obj.data.polygons.foreach_get("material_index", idx)
    assert all(a <= b for a, b in zip(idx, idx[1:])), "faces not sorted by material"
    assert sorted(set(idx)) == list(range(len(SLOT_ORDER))), "unused material slot"


def export_fbx(objs, path):
    """rh.export_fbx settings, with the UCX_ hulls in the same file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"}, use_mesh_modifiers=True,
        mesh_smooth_type="FACE", path_mode="AUTO", axis_forward="-Z", axis_up="Y",
        global_scale=1.0, apply_unit_scale=True, use_space_transform=True, bake_anim=False)


# -----------------------------------------------------------------------------
# Preview: desk, mouse pad and room, clearance check, renders (design frame)
# -----------------------------------------------------------------------------

# (fbx, real L_Office location cm, yaw deg)
LEVEL_REFERENCE = [
    (ART / "RetroComputer/Exports/SM_MousePad.fbx", (281.0, 222.0, -42.0), 0.0),
    (ART / "OfficeDesk/Exports/SM_OfficeDesk_A.fbx", (295.0, 300.0, -125.0), 180.0),
    (ART / "OfficeRoom/Exports/SM_OfficeRoom.fbx", (0.0, 0.0, 0.0), 0.0),
]


def load_reference():
    objs = []
    for path, world, yaw in LEVEL_REFERENCE:
        loc, local_yaw = level_to_bp(*world, yaw)
        before = set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(path))
        for o in set(bpy.data.objects) - before:
            if o.type != "MESH":
                continue
            # Blender BP space of the design frame: real BP-relative placement + design offset 0
            o.location = (loc[0] / 100.0, -loc[1] / 100.0, loc[2] / 100.0)
            o.rotation_euler = (0.0, 0.0, math.radians(-local_yaw))
            o.scale = (1.0, 1.0, 1.0)
            objs.append(o)
    bpy.context.view_layer.update()
    return objs


def clearance(refs):
    """Smallest distance from each cable centerline to the meshes (minus the cable radius), and cable to cable."""
    trees = []
    for o in refs:
        bm = bmesh.new()
        bm.from_mesh(o.data)
        bm.transform(o.matrix_world)
        trees.append((o.name, BVHTree.FromBMesh(bm)))
        bm.free()
    report = {}
    paths = {}
    for name, pts, radius, skip in CABLE_SETS:
        path = catmull_rom([W(*p) for p in pts], 0.008)
        paths[name] = (path, radius, skip)
        worst = (1e9, None, None)
        for p in path[skip:len(path) - skip]:
            for tname, tree in trees:
                hit = tree.find_nearest(p)
                if hit[0] is not None and hit[3] - radius / 100.0 < worst[0]:
                    worst = (hit[3] - radius / 100.0, tname, tuple(round(c * 100.0, 1) for c in p))
        report[name] = (round(worst[0] * 100.0, 2), worst[1], worst[2])
    names = list(paths)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            pa, ra, sa = paths[names[i]]
            pb, rb, sb = paths[names[j]]
            best = min((a - b).length - (ra + rb) / 100.0
                       for a in pa[sa:len(pa) - sa] for b in pb[sb:len(pb) - sb:2])
            report[names[i] + "/" + names[j]] = (round(best * 100.0, 2), "cable", None)
    return report


def render_views(out_dir):
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
        # name: (design world location, look-at, lens mm, ortho scale or 0)
        "seat": ((326.0, 167.0, -4.0), (332.0, 275.0, -40.0), 20.0, 0.0),
        "top": ((338.0, 262.0, 100.0), (338.0, 262.1, -42.0), 50.0, 1.2),
        "behind": ((340.0, 304.0, 20.0), (345.0, 290.0, -60.0), 14.0, 0.0),
        "under": ((330.0, 215.0, -105.0), (335.0, 300.0, -112.0), 16.0, 0.0),
        "mouse": ((272.0, 212.0, -27.0), (287.0, 236.0, -41.5), 35.0, 0.0),
        "mouse_side": ((262.0, 236.0, -36.0), (286.0, 234.0, -41.3), 50.0, 0.0),
        "keyboard": ((330.0, 214.0, -24.0), (347.0, 238.0, -41.5), 35.0, 0.0),
    }
    for name, (loc, tgt, lens, ortho) in views.items():
        cam_data.lens = lens
        cam_data.type = "ORTHO" if ortho > 0 else "PERSP"
        cam_data.ortho_scale = max(ortho, 0.01)
        cam_data.clip_start = 0.005
        a, b = W(*loc), W(*tgt)
        cam.location = a
        cam.rotation_euler = (b - a).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(out_dir / f"computer_{name}.png")
        bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.cameras.remove(cam_data)


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview = argv[argv.index("--preview") + 1] if "--preview" in argv else None
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Desk computer of L_Office: monitor, tower, keyboard, mouse and cables in one mesh.",
        Pivot="SM_ComputerMonitor pivot (Computer_BP Monitor component frame). Slot 1 = Screen.",
        ExportFBX=str(FBX))

    cables = build_cables()
    parts = place_parts()
    refs = []
    if preview:
        refs = load_reference()
        print(f"DESK_TOP {DESK_TOP:.3f}  PAD_TOP {PAD_TOP:.3f}  WALL_Y {WALL_Y:.3f}  (design frame)")
        for k, v in clearance(parts + refs).items():
            print(f"CLEARANCE {k}: {v[0]} cm to {v[1]} at {v[2]}")

    hulls = [support_hull(p, BP_TO_MONITOR, f"UCX_{NAME}_{i:02d}") for i, p in enumerate(parts)]
    maps = [(p, spec[4]) for p, spec in zip(parts, PARTS)] + [(cables, {})]
    mats = final_materials(maps)
    for obj, mapping in maps:
        remap_materials(obj, mapping, mats)
        assert len(obj.data.uv_layers) == 1, f"{obj.name} has {len(obj.data.uv_layers)} UV maps"
        obj.data.uv_layers[0].name = "UVMap"
    obj = join([p for p in parts] + [cables], NAME)
    obj.data.transform(BP_TO_MONITOR)
    obj.data.update()
    sort_faces(obj)
    for m in list(bpy.data.materials):
        if m.users == 0:
            bpy.data.materials.remove(m)
    rh.report(obj)
    for h in hulls:
        print(f"{h.name}: {len(h.data.vertices)} verts")

    if preview:
        obj.matrix_world = BP_TO_MONITOR.inverted()  # back into BP space for the design-frame cameras
        for h in hulls:
            h.hide_render = True
        render_views(preview)
        obj.matrix_world = Matrix.Identity(4)
        for h in hulls:
            h.hide_render = False
        for o in refs:
            bpy.data.objects.remove(o, do_unlink=True)

    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    for backup in HERE.glob("*.blend1"):
        backup.unlink()
    export_fbx([obj] + hulls, FBX)
    print(f"Saved {BLEND} and {FBX}")


main()
