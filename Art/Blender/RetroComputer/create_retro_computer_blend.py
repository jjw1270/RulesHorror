"""RulesHorror UE-ready retro (90s beige) computer set generator.

Builds six meshes in one .blend and exports one FBX per mesh:
  SM_KeyboardErgo     replaces Deko_MatrixDemo SM_ComputerKeyboard_A01_N1
  SM_KeyboardStandard replaces Deko_MatrixDemo SM_ComputerKeyboard_A02_N1
  SM_CRTMonitor       replaces Deko_MatrixDemo SM_ComputerMonitor_A02_N1
  SM_Mouse            replaces Deko_MatrixDemo SM_ComputerMouse_A01_N1
  SM_MousePad         replaces Deko_MatrixDemo SM_ComputerMousePad_A01_N1
  SM_PCTower          replaces Deko_MatrixDemo SM_ComputerTower_B01_N1

Every mesh keeps the original's bounds and pivot so placed actors (L_Office, Computer_BP) keep
their transforms. The user sits on the Unreal +Y side (= Blender -Y): monitor screen, tower front,
keyboard front edge face Blender -Y; mouse buttons point to Blender +Y (away from the user).

Run:
  blender --background --factory-startup --python create_retro_computer_blend.py
  (optional) ... --python create_retro_computer_blend.py -- --preview <dir>
"""

import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "RetroComputer.blend"
EXPORT_DIR = BASE_DIR / "Exports"

# Material slots: name -> (linear RGBA, metallic, roughness). Slot names become Unreal slot names.
MAT_DEFS = {
    "Plastic_Beige": ((0.62, 0.58, 0.48, 1.0), 0.0, 0.55),
    "Keys_Light": ((0.66, 0.64, 0.58, 1.0), 0.0, 0.45),
    "Keys_Dark": ((0.30, 0.29, 0.27, 1.0), 0.0, 0.45),
    "Screen": ((0.025, 0.034, 0.032, 1.0), 0.0, 0.08),
    "Plastic_Grey": ((0.30, 0.29, 0.27, 1.0), 0.0, 0.5),
    "Panel_Dark": ((0.03, 0.03, 0.03, 1.0), 0.0, 0.6),
    "LED": ((0.05, 0.45, 0.08, 1.0), 0.0, 0.3),
    "Plastic_OffWhite": ((0.70, 0.69, 0.64, 1.0), 0.0, 0.42),
    "Seam_Dark": ((0.04, 0.04, 0.04, 1.0), 0.0, 0.6),
    "Pad_Fabric": ((0.58, 0.58, 0.71, 1.0), 0.0, 0.9),
    "Pad_Foam": ((0.36, 0.36, 0.43, 1.0), 0.0, 0.85),
}


# -----------------------------------------------------------------------------
# Small math / geometry helpers (rh_prop_utils covers boxes, cylinders, merging)
# -----------------------------------------------------------------------------


def clamp01(t):
    return max(0.0, min(1.0, t))


def smoothstep(e0, e1, x):
    t = clamp01((x - e0) / (e1 - e0))
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def loft(loops, mat_index, cap_start=True, cap_end=True):
    """Quad strips between equally sized closed loops; optional n-gon caps (closed shell)."""
    bm = bmesh.new()
    rings = [[bm.verts.new(Vector(p)) for p in loop] for loop in loops]
    n = len(rings[0])
    for a, b in zip(rings, rings[1:]):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((a[i], a[j], b[j], b[i]))
    if cap_start:
        bm.faces.new(list(reversed(rings[0])))
    if cap_end:
        bm.faces.new(rings[-1])
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def rr_points(cx, cy, w, h, r, seg):
    """Rounded rectangle outline (2D), 4 * (seg + 1) points, counter-clockwise."""
    hw, hh = w * 0.5, h * 0.5
    r = max(1e-4, min(r, hw - 1e-4, hh - 1e-4))
    pts = []
    for ox, oy, a0 in ((hw - r, hh - r, 0.0), (-hw + r, hh - r, 90.0),
                       (-hw + r, -hh + r, 180.0), (hw - r, -hh + r, 270.0)):
        for k in range(seg + 1):
            a = math.radians(a0 + 90.0 * k / seg)
            pts.append((cx + ox + r * math.cos(a), cy + oy + r * math.sin(a)))
    return pts


def rr_xz(cx, cz, w, h, r, y, seg=4):
    """Rounded rectangle loop in the XZ plane at depth y (for things facing -Y)."""
    return [Vector((p[0], y, p[1])) for p in rr_points(cx, cz, w, h, r, seg)]


def rr_xy(cx, cy, w, d, r, z, seg=3):
    """Rounded rectangle loop in the XY plane at height z."""
    return [Vector((p[0], p[1], z)) for p in rr_points(cx, cy, w, d, r, seg)]


def oct_ring(w, d, c, z):
    hw, hd = w * 0.5, d * 0.5
    return [Vector(p) for p in (
        (hw, -hd + c, z), (hw, hd - c, z), (hw - c, hd, z), (-hw + c, hd, z),
        (-hw, hd - c, z), (-hw, -hd + c, z), (-hw + c, -hd, z), (hw - c, -hd, z))]


def keycap(w, d, h, mat_index, matrix):
    """Tapered keycap with chamfered corners and a chamfered top edge. Origin = bottom center."""
    t1, t2, c = 0.0011, 0.0024, 0.0012
    loops = [oct_ring(w, d, c, 0.0),
             oct_ring(w - 2 * t1, d - 2 * t1, c, h * 0.72),
             oct_ring(w - 2 * t2, d - 2 * t2, c * 0.8, h)]
    return rh.transform(loft(loops, mat_index), matrix)


def align_z_to(normal):
    return Vector((0.0, 0.0, 1.0)).rotation_difference(normal.normalized()).to_matrix().to_4x4()


def add_row(out, r, items, x=0.0, **extra):
    """Keyboard row helper. items: (width_u, dark[, depth_u]) or ("gap", width_u)."""
    for it in items:
        if it[0] == "gap":
            x += it[1]
            continue
        w, dark = it[0], it[1]
        d = it[2] if len(it) > 2 else 1.0
        rec = {"row": r, "x": x, "w": w, "d": d, "dark": dark}
        rec.update(extra)
        out.append(rec)
        x += w


def std_main_block(out, ergo=False, f_row=5.5):
    """Main alphanumeric block (15u wide). Rows 0 (space) .. 4 (numbers), F row at f_row."""
    if ergo:
        add_row(out, f_row, [(1, 1), ("gap", 0.5)] + [(1, 0)] * 5 + [("gap", 1.25)]
                + [(1, 0)] * 4 + [("gap", 0.25)] + [(1, 0)] * 3)
    else:
        add_row(out, f_row, [(1, 1), ("gap", 1)] + [(1, 0)] * 4 + [("gap", 0.5)]
                + [(1, 0)] * 4 + [("gap", 0.5)] + [(1, 0)] * 4)
    add_row(out, 4, [(1, 0)] * 13 + [(2, 1)])
    add_row(out, 3, [(1.5, 1)] + [(1, 0)] * 12 + [(1.5, 0)])
    add_row(out, 2, [(1.75, 1)] + [(1, 0)] * 11 + [(2.25, 1)])
    add_row(out, 1, [(2.25, 1)] + [(1, 0)] * 10 + [(2.75, 1)])
    if ergo:
        add_row(out, 0, [(1.25, 1)] * 3 + [(3.5, 0), (2.75, 0)] + [(1.25, 1)] * 4)
    else:
        add_row(out, 0, [(1.25, 1)] * 3 + [(6.25, 0)] + [(1.25, 1)] * 4)


def nav_and_numpad(out, f_row=5.5, x=0.0):
    """Nav cluster (3u) + 0.5u gap + numpad (4u), starting at x (u)."""
    add_row(out, f_row, [(1, 1)] * 3, x=x)
    add_row(out, 4, [(1, 1)] * 3, x=x)
    add_row(out, 3, [(1, 1)] * 3, x=x)
    add_row(out, 1, [("gap", 1), (1, 1)], x=x)
    add_row(out, 0, [(1, 1)] * 3, x=x)
    n = x + 3.5
    add_row(out, 4, [(1, 1)] * 4, x=n)
    add_row(out, 3, [(1, 0)] * 3, x=n)
    add_row(out, 2, [(1, 0)] * 3 + [(1, 1, 2)], x=n)
    add_row(out, 1, [(1, 0)] * 3, x=n)
    add_row(out, 0, [(2, 0), (1, 0), (1, 1, 2)], x=n)


# -----------------------------------------------------------------------------
# SM_KeyboardStandard (beige 104-key, slight typing angle)
# UE bounds: x -23.3..23.0, y -14.2..3.2, z -0.2..2.6 -> Blender y -0.032 (front) .. 0.142 (back)
# -----------------------------------------------------------------------------


def build_keyboard_standard(mats):
    U, GAP, KEY_H = 0.019, 0.0015, 0.0095
    XMIN, XMAX, YMIN, YMAX = -0.233, 0.230, -0.032, 0.142
    X0, Y0 = -0.221, -0.012  # main block left edge, row 0 front edge
    ZB, DECK, RIM = -0.002, 0.011, 0.0145
    SLOPE = 0.045  # typing angle: rise per meter toward the back (about 2.6 degrees)
    I_CASE, I_KL, I_KD = 0, 1, 2

    bm = bmesh.new()
    ocx, ocy, ow, od = (XMIN + XMAX) * 0.5, (YMIN + YMAX) * 0.5, XMAX - XMIN, YMAX - YMIN
    wx0, wx1 = X0 - 0.0012, X0 + 23 * U + 0.0012
    wy0, wy1 = Y0 - 0.0015, Y0 + 6.5 * U + 0.0015
    icx, icy, iw, idp = (wx0 + wx1) * 0.5, (wy0 + wy1) * 0.5, wx1 - wx0, wy1 - wy0
    # Case: key well floor -> inner wall -> rim top -> outer wall -> bottom (one closed shell).
    rh.merge_into(bm, loft([
        rr_xy(icx, icy, iw, idp, 0.002, DECK),
        rr_xy(icx, icy, iw, idp, 0.002, RIM - 0.0009),
        rr_xy(icx, icy, iw + 0.0018, idp + 0.0018, 0.0029, RIM),
        rr_xy(ocx, ocy, ow - 0.004, od - 0.004, 0.006, RIM),
        rr_xy(ocx, ocy, ow, od, 0.008, RIM - 0.002),
        rr_xy(ocx, ocy, ow, od, 0.008, ZB + 0.0015),
        rr_xy(ocx, ocy, ow - 0.003, od - 0.003, 0.0065, ZB),
    ], I_CASE))
    # Raised back strip with the indicator window above the numpad.
    rh.merge_into(bm, rh.beveled_box(XMIN + 0.005, XMAX - 0.005, wy1 + 0.0025, YMAX - 0.0035,
                                     RIM - 0.002, 0.0168, 0.0012, I_CASE, 2))
    rh.merge_into(bm, rh.beveled_box(0.150, 0.206, 0.1215, 0.1315, 0.015, 0.0173, 0.0005, I_KD, 1))
    # Three little indicator lenses.
    for i in range(3):
        cx = 0.160 + i * 0.018
        rh.merge_into(bm, rh.beveled_box(cx - 0.0025, cx + 0.0025, 0.1245, 0.1285, 0.016, 0.0176,
                                         0.0004, I_KL, 1))

    keys = []
    std_main_block(keys, ergo=False, f_row=5.5)
    nav_and_numpad(keys, f_row=5.5, x=15.5)
    for k in keys:
        cx = X0 + (k["x"] + k["w"] * 0.5) * U
        cy = Y0 + (k["row"] + k["d"] * 0.5) * U
        m = Matrix.Translation((cx, cy, DECK - 0.001))
        rh.merge_into(bm, keycap(k["w"] * U - GAP, k["d"] * U - GAP, KEY_H,
                                 I_KD if k["dark"] else I_KL, m))

    # Typing angle: wedge deformation, bottom stays flat, deck and keys rise toward the back.
    for v in bm.verts:
        wgt = clamp01((v.co.z - ZB) / (DECK - ZB))
        v.co.z += SLOPE * (v.co.y - YMIN) * wgt
    return rh.finish_object(bm, "SM_KeyboardStandard",
                            [mats["Plastic_Beige"], mats["Keys_Light"], mats["Keys_Dark"]])


# -----------------------------------------------------------------------------
# SM_KeyboardErgo (beige split ergonomic keyboard, tented, palm rest)
# UE bounds: x -25..25, y -12.2..11.8, z 0..4 -> Blender y -0.118 (front) .. 0.122 (back)
# -----------------------------------------------------------------------------


def build_keyboard_ergo(mats):
    U, GAP, KEY_H = 0.018, 0.0014, 0.0085
    TH = math.radians(10.0)  # rotation of each half (gap opens toward the back)
    G = 0.006  # gap between the two space bar halves
    FY = 5.4  # F row (u)
    A = 0.2475  # outline half width; the side wall adds 2.5 mm
    Y_FRONT, KEY_BACK = -0.1155, 0.1095
    R_END, END_K = 0.045, 0.55
    I_CASE, I_KL, I_KD = 0, 1, 2

    # --- key layout ------------------------------------------------------------------------
    main = []
    std_main_block(main, ergo=True, f_row=FY)
    split = {FY: 7.0, 4: 7.0, 3: 6.5, 2: 6.75, 1: 7.25, 0: 7.25}
    piv_x = 7.25 * U

    def half_tf(group):
        ang = TH if group == "L" else -TH
        off = -G * 0.5 if group == "L" else G * 0.5
        c, s = math.cos(ang), math.sin(ang)

        def f(xu, yu):
            px, py = xu * U - piv_x, yu * U
            return Vector((c * px - s * py + off, s * px + c * py))
        return f, ang

    tf = {"L": half_tf("L"), "R": half_tf("R")}
    placed = []  # (corners[4], center, yaw, w, d, dark)

    def place(k, f, yaw):
        x0, y0, w, d = k["x"], k["row"], k["w"], k["d"]
        corners = [f(x0, y0), f(x0 + w, y0), f(x0 + w, y0 + d), f(x0, y0 + d)]
        placed.append({"c": corners, "ctr": f(x0 + w * 0.5, y0 + d * 0.5), "yaw": yaw,
                       "w": w, "d": d, "dark": k["dark"]})

    for k in main:
        k["dark"] = 1 if (k["row"] == FY and k["x"] == 0.0) else 0
        g = "L" if k["x"] + k["w"] * 0.5 < split[k["row"]] else "R"
        f, yaw = tf[g]
        place(k, f, yaw)
        placed[-1]["g"] = g
    right_max = max(p.x for k in placed if k["g"] == "R" for p in k["c"])
    xn, yn = right_max + 0.6 * U, -1.0 * U
    nav = []
    nav_and_numpad(nav, f_row=FY, x=0.0)

    def fn(xu, yu):
        return Vector((xn + xu * U, yn + yu * U))
    for k in nav:
        place(k, fn, 0.0)
        placed[-1]["g"] = "N"
    # Indicator window above the numpad.
    led = {"x": 3.5 + 0.7, "row": 4.55, "w": 2.6, "d": 0.45, "dark": 1}

    # Center the key field in x, put the back of the keys at KEY_BACK.
    allp = [p for k in placed for p in k["c"]]
    shift = Vector((-(min(p.x for p in allp) + max(p.x for p in allp)) * 0.5,
                    KEY_BACK - max(p.y for p in allp)))
    for k in placed:
        k["c"] = [p + shift for p in k["c"]]
        k["ctr"] = k["ctr"] + shift
    split_x = tf["L"][0](7.25, 0.0).x + G * 0.5 + shift.x
    alpha_end = right_max + shift.x
    led_ctr = fn(led["x"] + led["w"] * 0.5, led["row"] + led["d"] * 0.5) + shift

    # --- outline functions from the key footprints ----------------------------------------
    STEP = 0.002
    bx = [-0.27 + i * STEP for i in range(int(0.54 / STEP) + 1)]
    kf = [None] * len(bx)
    kb = [None] * len(bx)
    for k in placed:
        c = k["c"]
        xmin, xmax = min(p.x for p in c), max(p.x for p in c)
        for i, x in enumerate(bx):
            if x < xmin or x > xmax:
                continue
            ys = []
            for a, b in zip(c, c[1:] + c[:1]):
                if (a.x - x) * (b.x - x) <= 0 and abs(b.x - a.x) > 1e-9:
                    ys.append(a.y + (x - a.x) * (b.y - a.y) / (b.x - a.x))
            if ys:
                kf[i] = min(ys) if kf[i] is None else min(kf[i], min(ys))
                kb[i] = max(ys) if kb[i] is None else max(kb[i], max(ys))
    valid = [i for i in range(len(bx)) if kf[i] is not None]
    for i in range(len(bx)):
        if kf[i] is None:
            j = min(valid, key=lambda v: abs(v - i))
            kf[i], kb[i] = kf[j], kb[j]

    def window(arr, rad, fn_):
        return [fn_(arr[max(0, i - rad):i + rad + 1]) for i in range(len(arr))]

    def smooth(arr, rad):
        return [sum(arr[max(0, i - rad):i + rad + 1]) / len(arr[max(0, i - rad):i + rad + 1])
                for i in range(len(arr))]
    kf_s = smooth(window(kf, 8, min), 12)
    kb_s = smooth(window(kb, 10, max), 16)

    def sample(arr, x):
        t = (x - bx[0]) / STEP
        i = max(0, min(len(arr) - 2, int(math.floor(t))))
        return lerp(arr[i], arr[i + 1], clamp01(t - i))

    def np_blend(x):
        return smoothstep(alpha_end - 0.012, alpha_end + 0.075, x)

    def front_raw(x):
        palm = Y_FRONT + 0.008 * math.exp(-((x - split_x) / 0.035) ** 2)
        return lerp(palm, sample(kf_s, x) - 0.013, np_blend(x))

    def back_raw(x):
        return sample(kb_s, x) + 0.010

    def end_round(x):
        t = (abs(x) - (A - R_END)) / R_END
        if t <= 0.0:
            return 0.0
        return 1.0 - math.sqrt(max(0.0, 1.0 - min(1.0, t) ** 2))

    def front(x):
        f, b = front_raw(x), back_raw(x)
        return f + (b - f) * 0.5 * END_K * end_round(x)

    def back(x):
        f, b = front_raw(x), back_raw(x)
        return b - (b - f) * 0.5 * END_K * end_round(x)

    def tent(x):
        return 0.5 * (1.0 + math.cos(math.pi * max(-1.0, min(1.0, (x - split_x) / 0.20))))

    def h_base(x, y):
        t = tent(x)
        deck = 0.012 + 0.020 * t
        palm = lerp(0.019 + 0.011 * t, deck, np_blend(x))
        d = sample(kf_s, x) - 0.004 - y
        return deck + (palm - deck) * smoothstep(0.0, 0.045, d)

    def h_case(x, y):
        ef = clamp01((y - front(x)) / 0.020)
        eb = clamp01((back(x) - y) / 0.010)
        ee = clamp01((A - abs(x)) / 0.012)
        drop = max(0.008 * (1 - ef) ** 2, 0.004 * (1 - eb) ** 2, 0.005 * (1 - ee) ** 2)
        return h_base(x, y) - drop

    # --- case: top grid + side wall + bottom --------------------------------------------------
    cols = []
    n_end = 5
    for k in range(n_end):
        cols.append(-A + R_END * (1 - math.cos(k / n_end * math.pi * 0.5)))
    mid_n = max(2, round(2 * (A - R_END) / 0.019))
    for k in range(mid_n + 1):
        cols.append(-(A - R_END) + k * 2 * (A - R_END) / mid_n)
    for k in reversed(range(n_end)):
        cols.append(A - R_END * (1 - math.cos(k / n_end * math.pi * 0.5)))
    palm_v = [0.0, 0.06, 0.16, 0.32, 0.55, 0.78, 1.0]
    key_v = [0.2, 0.4, 0.6, 0.8, 0.92, 1.0]

    bm = bmesh.new()
    grid = []
    for x in cols:
        f, b = front(x), back(x)
        k = min(max(sample(kf_s, x) - 0.006, f + 0.15 * (b - f)), b - 0.3 * (b - f))
        ys = [f + v * (k - f) for v in palm_v] + [k + v * (b - k) for v in key_v]
        grid.append([bm.verts.new((x, y, h_case(x, y))) for y in ys])
    nc, nr = len(cols) - 1, len(grid[0]) - 1
    for i in range(nc):
        for j in range(nr):
            bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    loop = ([grid[i][0] for i in range(nc + 1)] + [grid[nc][j] for j in range(1, nr + 1)]
            + [grid[i][nr] for i in range(nc - 1, -1, -1)] + [grid[0][j] for j in range(nr - 1, 0, -1)])
    n = len(loop)
    outs = []
    for i in range(n):
        d = loop[(i + 1) % n].co - loop[i - 1].co
        outs.append(Vector((d.y, -d.x, 0.0)).normalized())
    r1 = [bm.verts.new(v.co + o * 0.0022 - Vector((0, 0, 0.003))) for v, o in zip(loop, outs)]
    r3 = [bm.verts.new((v.co.x + o.x * 0.0018, v.co.y + o.y * 0.0018, 0.0)) for v, o in zip(loop, outs)]
    for ra, rb in ((loop, r1), (r1, r3)):
        for i in range(n):
            j = (i + 1) % n
            bm.faces.new((ra[i], ra[j], rb[j], rb[i]))
    bottom = bm.faces.new(list(reversed(r3)))
    bmesh.ops.triangulate(bm, faces=[bottom], quad_method="BEAUTY", ngon_method="BEAUTY")
    for f in bm.faces:
        f.material_index = I_CASE

    # --- keys on the tented deck -------------------------------------------------------------
    def deck_frame(x, y, yaw):
        e = 0.004
        dzdx = (h_base(x + e, y) - h_base(x - e, y)) / (2 * e)
        dzdy = (h_base(x, y + e) - h_base(x, y - e)) / (2 * e)
        nrm = Vector((-dzdx, -dzdy, 1.0))
        return (Matrix.Translation((x, y, h_base(x, y) - 0.0012)) @ align_z_to(nrm)
                @ Matrix.Rotation(yaw, 4, "Z"))

    for k in placed:
        m = deck_frame(k["ctr"].x, k["ctr"].y, k["yaw"])
        rh.merge_into(bm, keycap(k["w"] * U - GAP, k["d"] * U - GAP, KEY_H,
                                 I_KD if k["dark"] else I_KL, m))
    m = deck_frame(led_ctr.x, led_ctr.y, 0.0)
    rh.merge_into(bm, rh.transform(rh.beveled_box(-led["w"] * U * 0.5, led["w"] * U * 0.5,
                                                  -led["d"] * U * 0.5, led["d"] * U * 0.5,
                                                  0.0, 0.0028, 0.0005, I_KD, 1), m))
    return rh.finish_object(bm, "SM_KeyboardErgo",
                            [mats["Plastic_Beige"], mats["Keys_Light"], mats["Keys_Dark"]])


# -----------------------------------------------------------------------------
# SM_CRTMonitor (beige 90s CRT, screen faces Blender -Y = Unreal +Y)
# UE bounds: x -18.5..18.3, y -19.9..21.5, z -0.1..38 -> Blender y -0.215 (front) .. 0.199 (back)
# -----------------------------------------------------------------------------


def build_crt_monitor(mats):
    I_BODY, I_SCREEN, I_GREY = 0, 1, 2
    XC = -0.001
    YF = -0.213  # bezel front plane (buttons stick out to -0.215)
    ZB, ZT = 0.025, 0.380
    BW, BH = 0.368, ZT - ZB
    BCZ = (ZB + ZT) * 0.5
    OW, OH, OCZ = 0.306, 0.234, 0.2305  # screen opening

    bm = bmesh.new()
    rh.merge_into(bm, loft([
        rr_xz(XC, OCZ, OW - 0.016, OH - 0.016, 0.006, YF + 0.015),   # tunnel end (behind the glass)
        rr_xz(XC, OCZ, OW - 0.008, OH - 0.008, 0.008, YF + 0.006),
        rr_xz(XC, OCZ, OW, OH, 0.010, YF + 0.0012),
        rr_xz(XC, OCZ, OW + 0.003, OH + 0.003, 0.0115, YF),           # opening edge on the bezel face
        rr_xz(XC, BCZ, BW - 0.008, BH - 0.008, 0.016, YF),            # bezel face outer edge
        rr_xz(XC, BCZ, BW, BH, 0.020, YF + 0.004),
        rr_xz(XC, BCZ, BW - 0.002, BH - 0.002, 0.020, YF + 0.098),    # front shell back edge
        rr_xz(XC, BCZ, BW - 0.012, BH - 0.012, 0.020, YF + 0.101),    # seam step to the rear shell
        rr_xz(XC, BCZ + 0.004, BW - 0.020, BH - 0.022, 0.030, YF + 0.115),
        rr_xz(XC, BCZ + 0.010, 0.270, 0.245, 0.035, 0.118),           # tapered rear end
        rr_xz(XC, BCZ + 0.010, 0.252, 0.228, 0.030, 0.122),
        rr_xz(XC, BCZ + 0.012, 0.226, 0.204, 0.025, 0.194),           # neck box
        rr_xz(XC, BCZ + 0.012, 0.218, 0.196, 0.021, 0.199),
    ], I_BODY))

    # Curved glass: cushion-shaped slab, edges tucked into the tunnel walls.
    nx, nz = 12, 9
    gw, gh = OW - 0.006, OH - 0.006
    y_edge, bulge, back_y = YF + 0.0135, 0.007, YF + 0.0175
    gb = bmesh.new()
    grid = []
    for j in range(nz + 1):
        row = []
        for i in range(nx + 1):
            u, v = i / nx * 2 - 1, j / nz * 2 - 1
            row.append(gb.verts.new((XC + u * gw * 0.5, y_edge - bulge * (1 - u * u) * (1 - v * v),
                                     OCZ + v * gh * 0.5)))
        grid.append(row)
    for j in range(nz):
        for i in range(nx):
            gb.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
    border = ([grid[0][i] for i in range(nx + 1)] + [grid[j][nx] for j in range(1, nz + 1)]
              + [grid[nz][i] for i in range(nx - 1, -1, -1)] + [grid[j][0] for j in range(nz - 1, 0, -1)])
    backs = [gb.verts.new((v.co.x, back_y, v.co.z)) for v in border]
    for k in range(len(border)):
        k2 = (k + 1) % len(border)
        gb.faces.new((border[k], border[k2], backs[k2], backs[k]))
    gb.faces.new(list(reversed(backs)))
    for f in gb.faces:
        f.material_index = I_SCREEN
    rh.merge_into(bm, gb)

    # Controls under the screen.
    zc = 0.062
    rh.merge_into(bm, rh.cylinder(0.0105, 0.004, (XC + 0.135, YF + 0.0012, zc), "Y", I_GREY, 20))
    rh.merge_into(bm, rh.cylinder(0.0078, 0.005, (XC + 0.135, YF - 0.0002, zc), "Y", I_BODY, 20))
    rh.merge_into(bm, rh.cylinder(0.0018, 0.003, (XC + 0.112, YF + 0.0006, zc), "Y", I_GREY, 10))
    for i in range(4):
        x = XC + 0.018 + i * 0.019
        rh.merge_into(bm, rh.beveled_box(x - 0.0065, x + 0.0065, YF - 0.0016, YF + 0.003,
                                         zc - 0.003, zc + 0.003, 0.0008, I_GREY, 1))
    rh.merge_into(bm, rh.beveled_box(XC - 0.135, XC - 0.080, YF - 0.0006, YF + 0.003,
                                     zc - 0.0045, zc + 0.0045, 0.0006, I_GREY, 1))
    # Vent slots on top of the tapered rear shell.
    y8, z8 = YF + 0.115, BCZ + 0.004 + (BH - 0.022) * 0.5
    y9, z9 = 0.118, BCZ + 0.010 + 0.245 * 0.5
    ang = math.atan2(z9 - z8, y9 - y8)
    ym = -0.005
    zm = z8 + (z9 - z8) * (ym - y8) / (y9 - y8)
    for i in range(9):
        x = XC - 0.072 + i * 0.018
        slot = rh.box(x - 0.0022, x + 0.0022, -0.065, 0.065, -0.003, 0.0004, I_GREY)
        rh.merge_into(bm, rh.transform(slot, Matrix.Translation((0.0, ym, zm)) @ Matrix.Rotation(ang, 4, "X")))
    # Tilt/swivel stand.
    rh.merge_into(bm, rh.rounded_box(0.230, 0.200, -0.001, 0.010, 0.030, 0.003, I_BODY, center=(XC, -0.020)))
    rh.merge_into(bm, rh.rounded_box(0.150, 0.120, 0.008, 0.028, 0.020, 0.002, I_BODY, center=(XC, -0.030)))
    return rh.finish_object(bm, "SM_CRTMonitor", [mats["Plastic_Beige"], mats["Screen"], mats["Plastic_Grey"]])


# -----------------------------------------------------------------------------
# SM_Mouse (off-white two-button ball mouse)
# UE bounds: x -3.1..3.1, y -5.7..6.5, z 0..3.2 -> Blender y -0.065 (palm end) .. 0.057 (button end)
# -----------------------------------------------------------------------------


def build_mouse(mats):
    I_BODY, I_SEAM = 0, 1
    YB, YF = -0.065, 0.057
    HW, H, Z0 = 0.031, 0.0308, 0.0012
    N, K = 44, 9
    yc, b = (YB + YF) * 0.5, (YF - YB) * 0.5
    YH = yc - 0.010  # highest point, toward the palm
    raw = []
    for i in range(N):
        th = 2 * math.pi * i / N
        c, s = math.cos(th), math.sin(th)
        x = math.copysign(abs(c) ** (2 / 2.3), c)
        y = math.copysign(abs(s) ** (2 / 2.3), s)
        raw.append((x * (0.80 + 0.20 * (y + 1) * 0.5), yc + y * b))
    sx = HW / max(abs(p[0]) for p in raw)
    outline = [(p[0] * sx, p[1]) for p in raw]

    def prof(y):
        if y >= YH:
            t = (y - YH) / (YF - YH)
            return 1 - 0.40 * t * t
        t = (YH - y) / (YH - YB)
        return 1 - 0.22 * t * t

    def ring(r, zf):
        pts = []
        for px, py in outline:
            x, y = r * px, YH + r * (py - YH)
            pts.append((x, y, Z0 + zf * H * prof(y)))
        return pts

    bm = bmesh.new()
    bottom = [bm.verts.new((0.96 * px, yc + 0.96 * (py - yc), 0.0)) for px, py in outline]
    rings = [bottom]
    q = 2.4
    for k in range(K):
        a = k / K * math.pi * 0.5
        r = math.cos(a) ** (2 / q)
        zf = math.sin(a) ** (2 / q)
        rings.append([bm.verts.new(p) for p in ring(r, zf)])
    for ra, rb in zip(rings, rings[1:]):
        for i in range(N):
            j = (i + 1) % N
            bm.faces.new((ra[i], ra[j], rb[j], rb[i]))
    apex = bm.verts.new((0.0, YH, Z0 + H))
    for i in range(N):
        bm.faces.new((rings[-1][i], rings[-1][(i + 1) % N], apex))
    bm.faces.new(list(reversed(bottom)))
    for f in bm.faces:
        f.material_index = I_BODY
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.normal_update()
    bvh = BVHTree.FromBMesh(bm)

    def half_width(y):
        best = 0.0
        for (ax, ay), (bx_, by) in zip(outline, outline[1:] + outline[:1]):
            if (ay - y) * (by - y) <= 0 and abs(by - ay) > 1e-9:
                best = max(best, abs(ax + (y - ay) * (bx_ - ax) / (by - ay)))
        return best

    def seam(points_xy):
        hits = []
        for x, y in points_xy:
            loc, nrm, _, _ = bvh.ray_cast(Vector((x, y, 0.2)), Vector((0, 0, -1)))
            if loc is not None:
                hits.append((loc, nrm.normalized()))
        loops = []
        for i, (loc, nrm) in enumerate(hits):
            a, c = hits[max(i - 1, 0)][0], hits[min(i + 1, len(hits) - 1)][0]
            t = (c - a).normalized()
            side = t.cross(nrm).normalized()
            up = side.cross(t).normalized()
            hw, lift, depth = 0.00045, 0.00022, 0.0009
            loops.append([loc + side * hw + up * lift, loc - side * hw + up * lift,
                          loc - side * hw - up * depth, loc + side * hw - up * depth])
        return loft(loops, I_SEAM)

    y_s = YH + 0.024  # back edge of the buttons
    ws = 0.80 * half_width(y_s)
    rh.merge_into(bm, seam([(-ws + 2 * ws * i / 16, y_s) for i in range(17)]))
    for sgn in (-1, 1):
        pts = []
        for i in range(11):
            y = y_s + (YF - 0.006 - y_s) * i / 10
            pts.append((sgn * 0.80 * half_width(y), y))
        rh.merge_into(bm, seam(pts))
    rh.merge_into(bm, seam([(0.0, y_s + (YF - 0.0004 - y_s) * (i / 16) ** 0.7) for i in range(17)]))
    return rh.finish_object(bm, "SM_Mouse", [mats["Plastic_OffWhite"], mats["Seam_Dark"]])


# -----------------------------------------------------------------------------
# SM_MousePad (rounded rectangle, pale lavender fabric top on a foam base)
# UE bounds: x -14.2..14.2, y -11.2..11.2, z 0..1.0
# -----------------------------------------------------------------------------


def build_mouse_pad(mats):
    I_FABRIC, I_FOAM = 0, 1
    bm = rh.rounded_box(0.284, 0.224, 0.0, 0.010, 0.022, 0.0035, I_FOAM, edge_segments=3)
    bm.normal_update()
    for f in bm.faces:
        if f.calc_center_median().z > 0.0085 and f.normal.z > 0.6:
            f.material_index = I_FABRIC
    return rh.finish_object(bm, "SM_MousePad", [mats["Pad_Fabric"], mats["Pad_Foam"]])


# -----------------------------------------------------------------------------
# SM_PCTower (beige 90s tower, front panel faces Blender -Y = Unreal +Y)
# UE bounds: x -10.5..10.2, y -26.8..21.1, z 0..45.7 -> Blender y -0.211 (front) .. 0.268 (back)
# -----------------------------------------------------------------------------


def build_pc_tower(mats):
    I_BODY, I_DARK, I_LED = 0, 1, 2
    X0, X1 = -0.105, 0.102
    YF, YB, ZT = -0.209, 0.268, 0.457
    bm = bmesh.new()

    def add(part):
        rh.merge_into(bm, part)

    def fb(x0, x1, z0, z1, proud, bevel, mat, seg=1):
        add(rh.beveled_box(x0, x1, YF - proud, YF + 0.003, z0, z1, bevel, mat, seg))

    def bb(x0, x1, z0, z1, proud, bevel, mat, seg=1):
        add(rh.beveled_box(x0, x1, YB - 0.005, YB - 0.002 + proud, z0, z1, bevel, mat, seg))

    # Painted steel case + plastic front bezel.
    add(rh.beveled_box(X0, X1, -0.192, YB - 0.002, 0.010, ZT, 0.003, I_BODY, 2))
    add(rh.beveled_box(X0 + 0.0005, X1 - 0.0005, YF, -0.186, 0.004, ZT - 0.002, 0.005, I_BODY, 3))
    for x in (X0 + 0.022, X1 - 0.022):
        for y in (-0.165, 0.240):
            add(rh.cylinder(0.011, 0.012, (x, y, 0.006), "Z", I_DARK, 12))

    # Two 5.25" bays: CD-ROM drive and a second drive.
    fb(-0.094, 0.058, 0.349, 0.444, 0.0004, 0.0003, I_DARK)
    fb(-0.0928, 0.0568, 0.3982, 0.4428, 0.0013, 0.0008, I_BODY)
    fb(-0.0870, 0.0320, 0.4240, 0.4385, 0.0017, 0.0003, I_DARK)
    fb(-0.0860, 0.0310, 0.4250, 0.4375, 0.0023, 0.0006, I_BODY)
    fb(0.0400, 0.0500, 0.4050, 0.4100, 0.0025, 0.0006, I_BODY)
    fb(0.0280, 0.0325, 0.4065, 0.4085, 0.0018, 0.0002, I_LED)
    add(rh.cylinder(0.0022, 0.004, (-0.080, YF - 0.0002, 0.4075), "Y", I_DARK, 10))
    fb(-0.0928, 0.0568, 0.3502, 0.3948, 0.0013, 0.0008, I_BODY)
    fb(-0.0700, 0.0300, 0.3705, 0.3740, 0.0016, 0.0002, I_DARK)
    fb(0.0400, 0.0500, 0.3600, 0.3650, 0.0025, 0.0006, I_BODY)
    # 3.5" floppy bay.
    fb(-0.072, 0.036, 0.3085, 0.3405, 0.0004, 0.0003, I_DARK)
    fb(-0.0708, 0.0348, 0.3097, 0.3393, 0.0013, 0.0008, I_BODY)
    fb(-0.0500, 0.0220, 0.3255, 0.3285, 0.0016, 0.0002, I_DARK)
    fb(0.0200, 0.0290, 0.3135, 0.3175, 0.0025, 0.0006, I_BODY)
    fb(-0.0630, -0.0570, 0.3140, 0.3160, 0.0018, 0.0002, I_LED)
    # Power button, reset button, power/HDD LEDs.
    add(rh.cylinder(0.0105, 0.004, (0.0765, YF + 0.001, 0.420), "Y", I_DARK, 20))
    add(rh.cylinder(0.0085, 0.005, (0.0765, YF - 0.0005, 0.420), "Y", I_BODY, 20))
    add(rh.cylinder(0.0035, 0.005, (0.0765, YF + 0.0005, 0.393), "Y", I_DARK, 12))
    fb(0.0735, 0.0795, 0.3735, 0.3765, 0.0012, 0.0003, I_LED)
    fb(0.0735, 0.0795, 0.3635, 0.3665, 0.0012, 0.0003, I_LED)
    # Lower door panel.
    fb(-0.095, 0.092, 0.066, 0.296, 0.0012, 0.0010, I_BODY, 2)
    # Intake grille: dark backing with louvers.
    fb(-0.090, 0.087, 0.0155, 0.0545, 0.0004, 0.0003, I_DARK)
    for i in range(6):
        z0 = 0.0185 + i * 0.0058
        fb(-0.090, 0.087, z0, z0 + 0.0032, 0.0020, 0.0006, I_BODY)

    # Back: PSU with fan grille, expansion slot covers.
    bb(-0.090, 0.060, 0.365, 0.448, 0.0010, 0.0004, I_DARK)
    for i in range(6):
        z0 = 0.380 + i * 0.0105
        bb(-0.072, 0.012, z0, z0 + 0.003, 0.0018, 0.0004, I_BODY)
    bb(0.030, 0.052, 0.380, 0.395, 0.0016, 0.0004, I_BODY)
    bb(-0.090, 0.050, 0.065, 0.195, 0.0004, 0.0002, I_DARK)
    for i in range(7):
        x0 = -0.086 + i * 0.0195
        bb(x0, x0 + 0.014, 0.069, 0.191, 0.0012, 0.0003, I_BODY)
    return rh.finish_object(bm, "SM_PCTower", [mats["Plastic_Beige"], mats["Panel_Dark"], mats["LED"]])


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


REPLACES = {
    "SM_KeyboardErgo": "SM_ComputerKeyboard_A01_N1",
    "SM_KeyboardStandard": "SM_ComputerKeyboard_A02_N1",
    "SM_CRTMonitor": "SM_ComputerMonitor_A02_N1",
    "SM_Mouse": "SM_ComputerMouse_A01_N1",
    "SM_MousePad": "SM_ComputerMousePad_A01_N1",
    "SM_PCTower": "SM_ComputerTower_B01_N1",
}


def stats(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [-v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    return {"min": [round(min(xs) * 100, 2), round(min(ys) * 100, 2), round(min(zs) * 100, 2)],
            "max": [round(max(xs) * 100, 2), round(max(ys) * 100, 2), round(max(zs) * 100, 2)],
            "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons),
            "slots": [m.name for m in obj.data.materials]}


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview_dir = Path(args[args.index("--preview") + 1]) if "--preview" in args else None

    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacements for the Deko_MatrixDemo computer props (keyboards, CRT, mouse, pad, tower).",
        Pivot="Same pivot as each original. Unreal +Y (user side) = Blender -Y.",
        ExportDir=str(EXPORT_DIR))
    mats = {name: rh.make_material(name, c, m, r) for name, (c, m, r) in MAT_DEFS.items()}
    objs = [build_keyboard_ergo(mats), build_keyboard_standard(mats), build_crt_monitor(mats),
            build_mouse(mats), build_mouse_pad(mats), build_pc_tower(mats)]
    for name, mat in list(mats.items()):
        if mat.users == 0:
            bpy.data.materials.remove(mat)

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    for obj in objs:
        rh.export_fbx(obj, EXPORT_DIR / f"{obj.name}.fbx")
        rh.report(obj)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")

    if preview_dir is not None:
        info = {}
        for obj in objs:
            for o in objs:
                o.hide_render = o is not obj
            rh.preview_render(obj, preview_dir / f"{obj.name}.png")
            info[obj.name] = stats(obj)
        (preview_dir / "stats.json").write_text(json.dumps(info, indent=1))


if __name__ == "__main__":
    main()
