import sys
from pathlib import Path

import math

import bmesh
import bpy
from mathutils import Matrix

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready office door slab (painted steel door with a narrow wired-glass window).
# Replaces BrutalistLevelKit SM_Door_Basic3 (purchased asset).
# Same bounds and pivot as the original so the placed door keeps its transform:
#   pivot = hinge edge (x = 0) at floor level, centered in the slab thickness (y = 0).
#   The slab runs x 0 .. 101.13 cm (free edge at +X), z 4.33 .. 200 cm (floor gap below).
#   Unreal +Y = "outside" face (deep pull handle), Unreal -Y = room side (shallow pull, hinges).
#
# All layout numbers below are Unreal centimeters with Unreal Y. The finished bmesh is converted
# once at the end (x 0.01, y -0.01, z 0.01), so Blender +Y ends up as Unreal -Y as required.
#
# Optional preview renders (not saved into the .blend):
#   blender --background --factory-startup --python create_office_door_blend.py -- --preview <dir>

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "OfficeDoor.blend"
EXPORT_DIR = BASE_DIR / "Exports"

MESH_NAME = "SM_OfficeDoor"

# Material slots (slot names in Unreal come from these names).
MATERIALS = [
    # name, base color (linear), metallic, roughness
    ("Paint", (0.62, 0.62, 0.60, 1.0), 0.0, 0.55),
    ("Metal", (0.45, 0.45, 0.46, 1.0), 1.0, 0.40),
    ("Glass", (0.70, 0.78, 0.76, 0.45), 0.0, 0.55),
]
I_PAINT, I_METAL, I_GLASS = 0, 1, 2
GLASS_OPACITY = 0.45

# Slab (Unreal cm).
DOOR_X0, DOOR_X1 = 0.0, 101.13
DOOR_Z0, DOOR_Z1 = 4.33, 200.0
HALF_T = 2.2                    # 4.4 cm slab
SLAB_BEVEL = 0.5

# Window (centered on the slab width).
WIN_CX = (DOOR_X0 + DOOR_X1) * 0.5
OPEN_HW, OPEN_Z0, OPEN_Z1 = 11.5, 58.7, 166.1        # clear opening inside the metal lip
HOLE_HW, HOLE_Z0, HOLE_Z1 = 12.0, 58.2, 166.6        # cut-out in the slab (hidden inside the lip)
LIP_HW, LIP_Z0, LIP_Z1, LIP_Y = 13.0, 57.2, 167.6, 3.4   # through-going metal lip, both faces
FRAME_IN_HW, FRAME_IN_Z0, FRAME_IN_Z1 = 12.8, 57.4, 167.4  # inner edge of the face mouldings
FRAME_ROOM = (16.5, 54.7, 170.1)       # room side (-Y) moulding: half width, z0, z1 (wider, like the original)
FRAME_OUT = (14.6, 55.6, 169.2)        # outside (+Y) moulding
FRAME_Y = 3.0
BEAD_HW, BEAD_Z0, BEAD_Z1, BEAD_Y = 11.7, 95.0, 166.3, 1.9    # painted glazing panel around the glass
GLASS_HW, GLASS_Z0, GLASS_Z1 = 8.5, 99.0, 163.0                # visible glass
PANEL_Y = 1.4                                                  # recessed solid panel below the glass

# Pull handles (both faces, same height): round bar with domed ends on two posts with rosettes,
# screw caps on the bar front over the posts.
HANDLE_X = 92.0
GRIP_Z0, GRIP_Z1 = 63.5, 120.0
POST_Z = (66.0, 117.5)
POST_R, ROSE_R, CAP_PROUD = 1.25, 2.6, 0.1
# (face y, bar radius, outermost y) per side; the outermost y values are the original bounds.
HANDLE_OUT = (HALF_T, 2.0, 14.86)      # outside (+Y): deep pull
HANDLE_ROOM = (-HALF_T, 1.75, -8.75)   # room side (-Y): shallow pull

# Hinges on the room side (-Y), hinge edge x = 0.
HINGES = [(173.0, 193.0), (44.0, 63.5)]
KNUCKLE_X, KNUCKLE_R = 1.05, 1.0


# -----------------------------------------------------------------------------
# Geometry helpers (Unreal cm, Unreal Y)
# -----------------------------------------------------------------------------


def _on_rect(co, rect, eps=1e-4):
    x0, x1, z0, z1 = rect
    on_x = (abs(co.x - x0) < eps or abs(co.x - x1) < eps) and z0 - eps <= co.z <= z1 + eps
    on_z = (abs(co.z - z0) < eps or abs(co.z - z1) < eps) and x0 - eps <= co.x <= x1 + eps
    return on_x or on_z


def ring_solid(outer, inner, y0, y1, mat, bevel_outer=0.0, bevel_inner=0.0, seg=2):
    """Closed rectangular frame in the XZ plane (rect = x0, x1, z0, z1), extruded from y0 to y1.
    The faces are 4 mitred trapezoids per side, so outer and inner edges can be beveled separately."""
    bm = bmesh.new()

    def rect(r, y):
        x0, x1, z0, z1 = r
        return [bm.verts.new((x0, y, z0)), bm.verts.new((x1, y, z0)),
                bm.verts.new((x1, y, z1)), bm.verts.new((x0, y, z1))]

    of, ob = rect(outer, y0), rect(outer, y1)
    inf, inb = rect(inner, y0), rect(inner, y1)
    for k in range(4):
        n = (k + 1) % 4
        bm.faces.new((of[k], of[n], inf[n], inf[k]))
        bm.faces.new((ob[k], inb[k], inb[n], ob[n]))
        bm.faces.new((of[k], ob[k], ob[n], of[n]))
        bm.faces.new((inf[k], inf[n], inb[n], inb[k]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for r, off in ((outer, bevel_outer), (inner, bevel_inner)):
        if off > 0:
            edges = [e for e in bm.edges if all(_on_rect(v.co, r) for v in e.verts)]
            rh.bevel_edges(bm, edges, off, seg)
    for f in bm.faces:
        f.material_index = mat
    return bm


def lathe(profile, mat, segments=16):
    """Surface of revolution around +Z. profile = [(radius, height), ...] from bottom to top;
    radius 0 makes a pole, otherwise the ends are closed with flat caps."""
    bm = bmesh.new()
    rings = []
    for r, h in profile:
        if r <= 1e-9:
            rings.append([bm.verts.new((0.0, 0.0, h))])
        else:
            rings.append([bm.verts.new((r * math.cos(2 * math.pi * i / segments),
                                        r * math.sin(2 * math.pi * i / segments), h))
                          for i in range(segments)])
    for a, b in zip(rings, rings[1:]):
        for i in range(segments):
            j = (i + 1) % segments
            if len(a) == 1:
                bm.faces.new((a[0], b[j], b[i]))
            elif len(b) == 1:
                bm.faces.new((a[i], a[j], b[0]))
            else:
                bm.faces.new((a[i], a[j], b[j], b[i]))
    if len(rings[0]) > 1:
        bm.faces.new(list(reversed(rings[0])))
    if len(rings[-1]) > 1:
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        f.material_index = mat
    return bm


def place(bm, origin, direction="Z"):
    """Move a part built along +Z so its local origin sits at origin and +Z points along direction."""
    rot = {"Z": Matrix.Identity(4),
           "+Y": Matrix.Rotation(math.radians(-90.0), 4, "X"),
           "-Y": Matrix.Rotation(math.radians(90.0), 4, "X")}[direction]
    return rh.transform(bm, Matrix.Translation(origin) @ rot)


def part(bm, x0, x1, y0, y1, z0, z1, mat, bevel=0.0, seg=2):
    p = rh.beveled_box(x0, x1, y0, y1, z0, z1, bevel, mat, seg) if bevel > 0 else rh.box(x0, x1, y0, y1, z0, z1, mat)
    rh.merge_into(bm, p)


# -----------------------------------------------------------------------------
# Door parts
# -----------------------------------------------------------------------------


def build_slab(bm):
    outer = (DOOR_X0, DOOR_X1, DOOR_Z0, DOOR_Z1)
    hole = (WIN_CX - HOLE_HW, WIN_CX + HOLE_HW, HOLE_Z0, HOLE_Z1)
    rh.merge_into(bm, ring_solid(outer, hole, -HALF_T, HALF_T, I_PAINT, bevel_outer=SLAB_BEVEL))


def build_window(bm):
    cx = WIN_CX
    opening = (cx - OPEN_HW, cx + OPEN_HW, OPEN_Z0, OPEN_Z1)
    # Metal lip lining the cut-out, proud of both mouldings (stepped frame profile).
    lip = (cx - LIP_HW, cx + LIP_HW, LIP_Z0, LIP_Z1)
    rh.merge_into(bm, ring_solid(lip, opening, -LIP_Y, LIP_Y, I_METAL, bevel_outer=0.2, bevel_inner=0.3))
    # Face mouldings: wider on the room side, slimmer outside.
    frame_in = (cx - FRAME_IN_HW, cx + FRAME_IN_HW, FRAME_IN_Z0, FRAME_IN_Z1)
    for (hw, z0, z1), y0, y1 in ((FRAME_ROOM, -FRAME_Y, -(HALF_T - 0.4)),
                                 (FRAME_OUT, HALF_T - 0.4, FRAME_Y)):
        rh.merge_into(bm, ring_solid((cx - hw, cx + hw, z0, z1), frame_in, y0, y1, I_METAL, bevel_outer=0.3))
    # Painted glazing panel around the glass (its outer edge hides inside the lip).
    bead_outer = (cx - BEAD_HW, cx + BEAD_HW, BEAD_Z0, BEAD_Z1)
    glass_open = (cx - GLASS_HW, cx + GLASS_HW, GLASS_Z0, GLASS_Z1)
    rh.merge_into(bm, ring_solid(bead_outer, glass_open, -BEAD_Y, BEAD_Y, I_PAINT, bevel_inner=0.35))
    # Recessed solid panel under the glass.
    part(bm, cx - BEAD_HW, cx + BEAD_HW, -PANEL_Y, PANEL_Y, OPEN_Z0 - 0.2, BEAD_Z0 + 0.6, I_PAINT)
    # Frosted glass pane in the middle of the thickness (edges hidden in the glazing panel).
    part(bm, cx - GLASS_HW - 0.3, cx + GLASS_HW + 0.3, -0.25, 0.25, GLASS_Z0 - 0.3, GLASS_Z1 + 0.3, I_GLASS)


def build_handle(bm, face_y, grip_r, reach_y):
    """Pull handle: round bar with domed ends on two posts with chamfered rosettes."""
    sgn = 1.0 if reach_y > 0 else -1.0
    direction = "+Y" if sgn > 0 else "-Y"
    grip_y = reach_y - sgn * (grip_r + CAP_PROUD)
    depth = abs(grip_y - face_y)
    post_profile = [(0.0, -0.05), (ROSE_R, -0.05), (ROSE_R, 0.25), (ROSE_R - 0.3, 0.55), (ROSE_R - 0.65, 0.7),
                    (POST_R, 0.75), (POST_R, depth), (0.0, depth)]
    cap_profile = [(0.0, -0.3), (0.62, -0.3), (0.62, 0.0), (0.5, CAP_PROUD * 0.7), (0.0, CAP_PROUD)]
    for z in POST_Z:
        rh.merge_into(bm, place(lathe(post_profile, I_METAL, 16), (HANDLE_X, face_y, z), direction))
        rh.merge_into(bm, place(lathe(cap_profile, I_METAL, 10), (HANDLE_X, grip_y + sgn * grip_r, z), direction))
    l = GRIP_Z1 - GRIP_Z0
    r = grip_r
    grip_profile = [(0.0, 0.0), (r * 0.55, 0.05), (r * 0.85, 0.35), (r, 0.9),
                    (r, l - 0.9), (r * 0.85, l - 0.35), (r * 0.55, l - 0.05), (0.0, l)]
    rh.merge_into(bm, place(lathe(grip_profile, I_METAL, 16), (HANDLE_X, grip_y, GRIP_Z0)))


def build_hinge(bm, z0, z1):
    """Room-side butt hinge: leaf on the face, knuckle at the hinge edge, two screw heads."""
    fy = -HALF_T
    part(bm, 0.4, 5.2, fy - 0.35, fy + 0.05, z0, z1, I_METAL, 0.1, 1)
    knuckle = [(0.0, -1.0), (0.5, -0.95), (0.75, -0.6), (KNUCKLE_R, -0.5),
               (KNUCKLE_R, (z1 - z0) + 0.5), (0.75, (z1 - z0) + 0.6), (0.5, (z1 - z0) + 0.95),
               (0.0, (z1 - z0) + 1.0)]
    rh.merge_into(bm, place(lathe(knuckle, I_METAL, 12), (KNUCKLE_X, fy - KNUCKLE_R + 0.05, z0)))
    screw = [(0.0, -0.05), (0.45, -0.05), (0.45, 0.1), (0.3, 0.2), (0.0, 0.24)]
    for z in (z0 + 3.0, z1 - 3.0):
        rh.merge_into(bm, place(lathe(screw, I_METAL, 8), (3.4, fy - 0.35, z), "-Y"))


def build_door():
    bm = bmesh.new()
    build_slab(bm)
    build_window(bm)
    build_handle(bm, *HANDLE_OUT)
    build_handle(bm, *HANDLE_ROOM)
    for z0, z1 in HINGES:
        build_hinge(bm, z0, z1)
    # Unreal cm (Unreal Y) -> Blender meters (Blender +Y = Unreal -Y). Normals are recalculated in finish_object.
    rh.transform(bm, Matrix.Diagonal((0.01, -0.01, 0.01, 1.0)))
    return bm


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def preview_dir():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    if "--preview" in argv:
        return Path(argv[argv.index("--preview") + 1])
    return None


def make_materials():
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    glass = mats[I_GLASS]
    bsdf = next(n for n in glass.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Alpha"].default_value = GLASS_OPACITY
    glass.surface_render_method = "BLENDED"
    return mats


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for BrutalistLevelKit SM_Door_Basic3 (office door slab).",
        Pivot="Hinge edge (x=0) at floor level, center of the slab thickness. Slab x 0 .. 1.0113 m, "
              "z 0.0433 .. 2.0 m. Deep handle faces Blender -Y (Unreal +Y), hinges on Blender +Y (Unreal -Y).",
    )
    mats = make_materials()
    obj = rh.finish_object(build_door(), MESH_NAME, mats, uv_scale=1.0, sharp_angle_deg=35.0)

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, EXPORT_DIR / f"{MESH_NAME}.fbx")
    rh.report(obj)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()

    out = preview_dir()
    if out is not None:
        rh.preview_render(obj, out / f"{MESH_NAME}.png")
        rh.preview_render(obj, out / f"{MESH_NAME}_inside.png", azimuth_deg=-65.0)
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
