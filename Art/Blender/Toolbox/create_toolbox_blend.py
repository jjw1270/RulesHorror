import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

# RulesHorror UE-ready open tool tote (blue painted boards, center handle bar) with tools.
# Replaces /Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_Toolbox_Prestaged_A01_N1 (purchased asset),
# used by /Game/Objects/Interactable/ToolBox_BP.
# Same bounds and pivot as the original so the Blueprint keeps working:
#   pivot = bottom center of the box, Z up, long side along X, handle bar along X.
#   Unreal bounds X -27.7..26.5, Y -20.7..15.0, Z 0..22.9 cm (tools stick out on -X, +X and -Y).
# Everything below is laid out in Unreal centimeters; U() converts to Blender meters (Y flipped).

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "Toolbox.blend"
EXPORT_DIR = BASE_DIR / "Exports"
MESH_NAME = "SM_Toolbox"

# Material slots (slot names in Unreal come from these names). Linear colors.
MAT_BOX = "Paint_Blue"
MAT_STEEL = "Steel_Tools"
MAT_GRIP = "Grip_Yellow"
MAT_RUBBER = "Rubber_Black"
MATERIALS = [
    (MAT_BOX, (0.055, 0.270, 0.400, 1.0), 0.0, 0.55),
    (MAT_STEEL, (0.60, 0.60, 0.62, 1.0), 1.0, 0.30),
    (MAT_GRIP, (0.62, 0.47, 0.13, 1.0), 0.0, 0.40),
    (MAT_RUBBER, (0.018, 0.018, 0.020, 1.0), 0.0, 0.70),
]
I_BOX, I_STEEL, I_GRIP, I_RUBBER = 0, 1, 2, 3

# Box (Unreal cm).
LEN, WID = 50.0, 30.0
WALL_H, BOARD = 11.0, 1.2
DIVIDER_H = 14.5
POST_TOP = 22.9
BAR_Z, BAR_R = 21.4, 1.2


def U(x, y, z):
    """Unreal cm -> Blender m."""
    return Vector((x / 100.0, -y / 100.0, z / 100.0))


def ubox(x0, x1, y0, y1, z0, z1, mat, bevel=0.3):
    return rh.beveled_box(x0 / 100.0, x1 / 100.0, -y1 / 100.0, -y0 / 100.0, z0 / 100.0, z1 / 100.0,
                          bevel / 100.0, mat, segments=1)


def orient(p0, direction, xdir):
    """Matrix mapping local +Z onto `direction` and local +X as close as possible to `xdir`
    (all Blender coords), origin at p0."""
    d = direction.normalized()
    x = xdir - d * xdir.dot(d)
    if x.length < 1e-6:
        x = Vector((1.0, 0.0, 0.0)) - d * d.x
    x.normalize()
    y = d.cross(x)
    return Matrix(((x.x, y.x, d.x, p0.x), (x.y, y.y, d.y, p0.y), (x.z, y.z, d.z, p0.z), (0.0, 0.0, 0.0, 1.0)))


def UD(x, y, z):
    """Unreal direction -> Blender direction."""
    return Vector((x, -y, z))


def lathe_z(profile, segments):
    """Closed surface of revolution around local Z. profile: [(r, z, mat_of_next_segment)], r=0 = pole."""
    bm = bmesh.new()
    rings = []
    for r, z, _ in profile:
        if r < 1e-9:
            rings.append([bm.verts.new((0.0, 0.0, z))])
        else:
            rings.append([bm.verts.new((r * math.cos(2 * math.pi * k / segments),
                                        r * math.sin(2 * math.pi * k / segments), z)) for k in range(segments)])
    for i in range(len(profile) - 1):
        a, b, mat = rings[i], rings[i + 1], profile[i][2]
        for k in range(segments):
            k2 = (k + 1) % segments
            if len(a) == 1:
                f = bm.faces.new((a[0], b[k2], b[k]))
            elif len(b) == 1:
                f = bm.faces.new((a[k], a[k2], b[0]))
            else:
                f = bm.faces.new((a[k], a[k2], b[k2], b[k]))
            f.material_index = mat
    return bm


def lbox(x0, x1, y0, y1, z0, z1, mat, bevel=0.0):
    """Local-space (meters) box for tool parts."""
    if bevel > 0:
        return rh.beveled_box(x0, x1, y0, y1, z0, z1, bevel, mat, segments=1)
    return rh.box(x0, x1, y0, y1, z0, z1, mat)


def place(bm, matrix):
    return rh.transform(bm, matrix)


# -----------------------------------------------------------------------------
# Box
# -----------------------------------------------------------------------------


def toolbox_body():
    bm = bmesh.new()
    hx, hy = LEN / 2, WID / 2
    rh.merge_into(bm, ubox(-hx, hx, -hy, hy, 0.0, BOARD, I_BOX))                       # bottom
    rh.merge_into(bm, ubox(-hx, hx, hy - BOARD, hy, 0.0, WALL_H, I_BOX))               # front (+Y)
    rh.merge_into(bm, ubox(-hx, hx, -hy, -hy + BOARD, 0.0, WALL_H, I_BOX))             # back
    for sx in (-1, 1):
        x0, x1 = sorted((sx * hx, sx * (hx - BOARD)))
        rh.merge_into(bm, ubox(x0, x1, -hy + BOARD, hy - BOARD, 0.0, WALL_H, I_BOX))  # ends
        p0, p1 = sorted((sx * (hx - BOARD), sx * (hx - BOARD - 2.2)))
        rh.merge_into(bm, ubox(p0, p1, -1.8, 1.8, BOARD, POST_TOP, I_BOX, bevel=0.45))  # end posts
    rh.merge_into(bm, ubox(-hx + BOARD, hx - BOARD, -0.6, 0.6, BOARD, DIVIDER_H, I_BOX))  # center divider
    # Steel handle bar through the posts.
    rh.merge_into(bm, rh.cylinder(BAR_R / 100.0, (LEN - 2 * BOARD - 0.6) / 100.0, U(0.0, 0.0, BAR_Z),
                                  axis="X", mat_index=I_STEEL, segments=14))
    return bm


# -----------------------------------------------------------------------------
# Tools (built along local +Z in meters, then oriented)
# -----------------------------------------------------------------------------


def screwdriver(shaft_len=0.115, handle_len=0.088):
    z1 = shaft_len
    prof = [(0.0, 0.0, I_STEEL), (0.0018, 0.0004, I_STEEL), (0.0034, 0.012, I_STEEL),
            (0.0034, z1, I_STEEL), (0.0060, z1, I_RUBBER), (0.0060, z1 + 0.008, I_RUBBER),
            (0.0095, z1 + 0.010, I_GRIP), (0.0132, z1 + 0.022, I_GRIP), (0.0136, z1 + handle_len - 0.012, I_GRIP),
            (0.0118, z1 + handle_len - 0.003, I_GRIP), (0.0070, z1 + handle_len, I_GRIP), (0.0, z1 + handle_len, I_GRIP)]
    return lathe_z(prof, 10)


def claw_hammer():
    """Handle along +Z (butt at 0), head across local X at the top."""
    L = 0.24
    bm = lathe_z([(0.0, 0.0, I_RUBBER), (0.0125, 0.002, I_RUBBER), (0.0140, 0.020, I_RUBBER),
                  (0.0135, 0.110, I_RUBBER), (0.0100, 0.125, I_STEEL), (0.0085, L - 0.02, I_STEEL),
                  (0.0085, L, I_STEEL), (0.0, L, I_STEEL)], 10)
    # Head: round striking face on -X, neck block, split claw curving down on +X.
    face = rh.cylinder(0.0135, 0.045, (-0.032, 0.0, L - 0.006), axis="X", mat_index=I_STEEL, segments=12)
    rh.merge_into(bm, face)
    rh.merge_into(bm, lbox(-0.012, 0.022, -0.011, 0.011, L - 0.021, L + 0.009, I_STEEL, bevel=0.002))
    for sy in (-1, 1):
        claw = lbox(0.0, 0.055, -0.0035, 0.0035, -0.006, 0.006, I_STEEL, bevel=0.0012)
        rh.transform(claw, Matrix.Translation((0.020, sy * 0.0055, L - 0.004))
                     @ Matrix.Rotation(math.radians(-22.0), 4, "Y"))
        rh.merge_into(bm, claw)
    return bm


def adjustable_wrench():
    """Flat in local XZ (thickness along local Y), handle from z=0 up to the jaws."""
    bm = lbox(-0.0135, 0.0135, -0.0045, 0.0045, 0.0, 0.185, I_STEEL, bevel=0.0025)
    rh.merge_into(bm, lbox(-0.030, 0.019, -0.0065, 0.0065, 0.178, 0.218, I_STEEL, bevel=0.003))    # head
    rh.merge_into(bm, lbox(-0.030, -0.013, -0.0060, 0.0060, 0.214, 0.250, I_STEEL, bevel=0.0025))  # fixed jaw
    rh.merge_into(bm, lbox(0.002, 0.017, -0.0060, 0.0060, 0.214, 0.244, I_STEEL, bevel=0.0025))    # movable jaw
    rh.merge_into(bm, rh.cylinder(0.0055, 0.015, (-0.005, 0.0, 0.195), axis="Y", mat_index=I_STEEL, segments=8))
    rh.merge_into(bm, rh.cylinder(0.0055, 0.0098, (0.0, 0.0, 0.014), axis="Y", mat_index=I_RUBBER, segments=8))
    return bm


def pliers():
    """Jaws up (+Z), handles splayed along local X, grips yellow."""
    bm = bmesh.new()
    for sx in (-1, 1):
        a, b = Vector((sx * 0.024, 0.0, 0.0)), Vector((sx * 0.005, 0.0, 0.135))
        grip = rh.cylinder(0.0088, 0.100, (0.0, 0.0, 0.050), axis="Z", mat_index=I_GRIP, segments=8)
        rh.merge_into(grip, lbox(-0.0045, 0.0045, -0.0035, 0.0035, 0.095, (b - a).length, I_STEEL))
        rh.transform(grip, orient(a, b - a, Vector((1.0, 0.0, 0.0))))
        rh.merge_into(bm, grip)
    rh.merge_into(bm, rh.cylinder(0.0105, 0.013, (0.0, 0.0, 0.138), axis="Y", mat_index=I_STEEL, segments=10))
    rh.merge_into(bm, rh.cylinder(0.0090, 0.055, (0.0, 0.0, 0.170), axis="Z", mat_index=I_STEEL, segments=6,
                                  radius2=0.0028))
    return bm


def pry_bar(length=0.30):
    """Flat bar (width along local X), hook bending toward local -Y at the far end."""
    bm = lbox(-0.0090, 0.0090, -0.0038, 0.0038, 0.0, length, I_STEEL, bevel=0.0012)
    hook = lbox(-0.0090, 0.0090, -0.0038, 0.0038, 0.0, 0.035, I_STEEL, bevel=0.0012)
    rh.transform(hook, Matrix.Translation((0.0, 0.0, length - 0.004)) @ Matrix.Rotation(math.radians(40.0), 4, "X"))
    rh.merge_into(bm, hook)
    return bm


def flat_file(blade=0.20):
    bm = lbox(-0.0100, 0.0100, -0.0024, 0.0024, 0.0, blade, I_STEEL, bevel=0.0008)
    rh.merge_into(bm, lbox(-0.0030, 0.0030, -0.0018, 0.0018, blade, blade + 0.015, I_STEEL))
    rh.merge_into(bm, lathe_z([(0.0, blade + 0.012, I_GRIP), (0.0085, blade + 0.013, I_GRIP),
                               (0.0110, blade + 0.050, I_GRIP), (0.0085, blade + 0.088, I_GRIP),
                               (0.0, blade + 0.090, I_GRIP)], 8))
    return bm


def tools():
    bm = bmesh.new()
    X, Y = UD(1, 0, 0), UD(0, 1, 0)
    # Screwdriver leaning out over the left end (front compartment).
    sd = screwdriver()
    rh.transform(sd, orient(U(-13.6, 6.3, 1.3), UD(-13.8, 2.3, 15.8), X))
    rh.merge_into(bm, sd)
    # Screwdriver resting on the back wall rim, handle sticking out behind the box (right side).
    sd = screwdriver(shaft_len=0.120, handle_len=0.095)
    rh.transform(sd, orient(U(17.0, -8.0, 1.3), UD(0.08, -0.53, 0.848), X))
    rh.merge_into(bm, sd)
    # Claw hammer leaning head-up in the back compartment toward the left end, claw pointing down.
    hm = claw_hammer()
    rh.transform(hm, orient(U(-2.0, -8.4, 1.3), UD(-0.75, 0.0, 0.65), UD(-1, 0, -1)))
    rh.merge_into(bm, hm)
    # Adjustable wrench leaning in the back compartment, flat side to the front.
    wr = adjustable_wrench()
    rh.transform(wr, orient(U(-0.5, -11.0, 1.3), UD(-8.5, 5.5, 16.5), X))
    rh.merge_into(bm, wr)
    # Pliers standing in the back compartment, center-right.
    pl = pliers()
    rh.transform(pl, orient(U(6.0, -10.0, 1.3), UD(-1.5, 4.0, 16.0), X))
    rh.merge_into(bm, pl)
    # Pry bar lying across the front compartment, resting on the right end wall, hook down outside.
    pb = pry_bar()
    rh.transform(pb, orient(U(-3.0, 4.2, 1.7), UD(27.5, 1.4, 10.5), Y))
    rh.merge_into(bm, pb)
    # Flat file lying on the floor of the front compartment.
    ff = flat_file()
    rh.transform(ff, orient(U(-12.0, 10.5, 1.7), UD(25.0, -2.3, 1.9), Y))
    rh.merge_into(bm, ff)
    return bm


def create_mesh(mats):
    bm = toolbox_body()
    rh.merge_into(bm, tools())
    return rh.finish_object(bm, MESH_NAME, mats, sharp_angle_deg=40.0)


def main():
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for Deko_MatrixDemo SM_Toolbox_Prestaged_A01_N1 (open tool tote).",
        Pivot="Bottom center of the box, Z up, handle bar along X. Same bounds as the original.",
    )
    mats = [rh.make_material(n, c, m, r) for n, c, m, r in MATERIALS]
    obj = create_mesh(mats)
    rh.report(obj)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    rh.export_fbx(obj, EXPORT_DIR / f"{MESH_NAME}.fbx")
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")


if __name__ == "__main__":
    main()
