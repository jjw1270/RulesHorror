"""RulesHorror first-person arms (skeletal mesh) generator for Blender 5.1.

Builds SKM_FPArms: both forearms/hands of an office worker (pale blue-white dress shirt with buttoned cuffs,
pale skin) rigged to a UE-style skeleton, plus two looping actions (Idle, Walk), and exports one FBX.

Camera-space contract: the mesh is attached to the first-person camera with an identity relative transform,
so the object origin IS the eye point. Unreal +X = view forward, +Y = right, +Z = up.
Axis mapping: Blender +X -> UE +X, Blender +Y -> UE -Y, Blender +Z -> UE +Z. The player's RIGHT arm
(UE +Y) is therefore modeled on Blender -Y and its bones end in `_r`.

How it is built:
  1. A low-poly cage of the right arm is generated in a canonical, straight pose (wrist joint at the origin,
     hand along +X, back of the hand +Z, thumb side +Y, fingers straight), with per-vertex bone weights.
  2. One Catmull-Clark level is applied (weights are interpolated by the subdivision).
  3. The mesh is deformed (linear blend skinning) from the canonical pose into the final "cautious" pose:
     two-bone IK from shoulder to wrist, hand orientation, relaxed finger curl.
  4. The left arm is the mirror image. The armature is created directly in the final pose, so the rest pose
     equals the idle pose.

Regenerate:
  "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup \
      --python create_fp_arms_blend.py [-- --previews] [--agent-json <path>]
"""

import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Euler, Matrix, Quaternion, Vector

BASE_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(BASE_DIR.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

REPO_DIR = BASE_DIR.parent.parent.parent
OUTPUT_BLEND = BASE_DIR / "FPArms.blend"
EXPORT_DIR = BASE_DIR / "Exports"
OUTPUT_FBX = EXPORT_DIR / "SKM_FPArms.fbx"
PREVIEW_DIR = REPO_DIR / "Saved" / "ClaudeTmp" / "previews" / "FPArms"

MESH_NAME = "SKM_FPArms"
ARMATURE_NAME = "Armature"  # Unreal skips a node with this exact name, so `root` stays the root bone.
FPS = 30
ACTION_FRAMES = {"Idle": 120, "Walk": 30}

# Material slots (Blender material names become Unreal slot names).
MAT_SLEEVE, MAT_SKIN, MAT_BUTTON = "Sleeve", "Skin", "Button"
MATERIALS = [
    # name, linear base color, metallic, roughness, unreal family, grime
    (MAT_SLEEVE, (0.55, 0.60, 0.66, 1.0), 0.0, 0.80, "opaque", 0.1),
    (MAT_SKIN, (0.55, 0.38, 0.30, 1.0), 0.0, 0.55, "opaque", 0.0),
    (MAT_BUTTON, (0.10, 0.10, 0.105, 1.0), 0.0, 0.35, "opaque", 0.1),
]
I_SLEEVE, I_SKIN, I_BUTTON = 0, 1, 2

# -----------------------------------------------------------------------------
# Canonical right arm (meters). Wrist joint at the origin, forearm along -X, hand along +X,
# back of the hand +Z, thumb side +Y.
# -----------------------------------------------------------------------------

UPPERARM_LEN = 0.30
LOWERARM_LEN = 0.26
HAND_LEN = 0.090

INFL = 1.13  # cage inflation for skin cross sections (one subdivision level shrinks round tubes)
SLEEVE_INFL = 1.03

# Forearm skin rings (inside the sleeve): x, half width, half height (top), half height (bottom), centre z.
FOREARM_RINGS = [
    (-0.100, 0.0335, 0.0245, 0.0245, -0.002),
    (-0.022, 0.0292, 0.0190, 0.0190, -0.001),
]
# Palm rings: x, half width, top, bottom, centre z, superellipse exponent, thenar pad, hypothenar pad.
PALM_RINGS = [
    (0.002, 0.0285, 0.0168, 0.0172, -0.001, 2.2, 0.00, 0.00),
    (0.024, 0.0335, 0.0156, 0.0185, -0.002, 2.8, 0.22, 0.10),
    (0.047, 0.0385, 0.0142, 0.0172, -0.002, 3.2, 0.14, 0.12),
    (0.068, 0.0398, 0.0128, 0.0150, -0.001, 3.6, 0.00, 0.08),
]

FINGER_ORDER = ["pinky", "ring", "middle", "index"]  # ulnar -> radial
FINGERS = {
    # y, x of MCP joint, z, splay (deg, + = toward thumb), (proximal, middle, distal) lengths, size scale
    "pinky": (-0.0285, 0.079, -0.003, -9.0, (0.032, 0.021, 0.020), 0.80),
    "ring": (-0.0110, 0.088, 0.000, -3.5, (0.041, 0.027, 0.022), 0.93),
    "middle": (0.0075, 0.092, 0.001, 0.5, (0.045, 0.029, 0.023), 1.00),
    "index": (0.0265, 0.089, 0.000, 5.0, (0.041, 0.025, 0.022), 0.98),
}
FINGER_BASE = (0.0096, 0.0085)  # half width / half height at the knuckle for size scale 1
# Knuckle ring: palm side of each finger base sits further out than the dorsal knuckle.
KNUCKLE_DX = [0.009, 0.000, 0.009, 0.015, 0.010, 0.015]  # DL, T, DR, PR, B, PL
FINGER_STATIONS = [
    # phalanx, fraction of phalanx, half width, half height, centre dz
    (0, 0.45, 0.0090, 0.0080, -0.0004),
    (0, 1.00, 0.0092, 0.0083, 0.0000),
    (1, 0.50, 0.0081, 0.0071, -0.0004),
    (1, 1.00, 0.0080, 0.0069, 0.0000),
    (2, 0.45, 0.0076, 0.0062, -0.0004),
    (2, 0.80, 0.0067, 0.0055, -0.0006),
    (2, 0.97, 0.0040, 0.0032, -0.0010),
]

THUMB_CMC = Vector((0.019, 0.019, -0.009))
THUMB_DIR = Vector((0.78, 0.52, -0.26)).normalized()
THUMB_NAIL_HINT = Vector((0.0, 1.0, 0.15))  # thumbnail faces sideways (thumb is pronated vs. the fingers)
THUMB_LENS = (0.044, 0.030, 0.025)  # metacarpal, proximal, distal
THUMB_T1_BLEND = 0.6  # first thumb ring: blend from the palm opening toward the bone axis
THUMB_PALM_RINGS = (1, 2, 3)  # palm rings (index into PALM_RINGS) bordering the thumb opening
THUMB_STATIONS = [
    # phalanx, fraction, half width (across nail), half height (nail -> pad)
    (0, 0.72, 0.0135, 0.0118),
    (0, 1.00, 0.0108, 0.0098),
    (1, 0.50, 0.0094, 0.0086),
    (1, 1.00, 0.0101, 0.0085),
    (2, 0.45, 0.0104, 0.0076),
    (2, 0.80, 0.0092, 0.0064),
    (2, 0.97, 0.0050, 0.0036),
]

# Sleeve (12 verts per ring around the forearm axis). x, half width, half height, centre z, fold amount, weights.
SLEEVE_N = 16
SLEEVE_RINGS = [
    (-0.420, 0.058, 0.056, 0.000, 0.00, {"upperarm": 1.0}),
    (-0.300, 0.057, 0.055, 0.000, 0.03, {"upperarm": 0.85, "lowerarm": 0.15}),
    (-0.260, 0.055, 0.052, 0.000, 0.03, {"upperarm": 0.5, "lowerarm": 0.5}),
    (-0.220, 0.054, 0.050, 0.000, 0.04, {"upperarm": 0.15, "lowerarm": 0.85}),
    (-0.165, 0.052, 0.048, 0.000, 0.04, {"lowerarm": 1.0}),
    (-0.120, 0.0490, 0.0450, 0.000, 0.05, {"lowerarm": 1.0}),
    (-0.090, 0.0465, 0.0410, -0.001, 0.045, {"lowerarm": 1.0}),
    (-0.075, 0.0418, 0.0335, -0.001, 0.0, {"lowerarm": 1.0}),   # cuff seam
    (-0.046, 0.0412, 0.0330, -0.001, 0.0, {"lowerarm": 1.0}),
    (-0.019, 0.0407, 0.0326, -0.001, 0.0, {"lowerarm": 1.0}),   # cuff end (outside)
    (-0.0145, 0.0392, 0.0311, -0.001, 0.0, {"lowerarm": 1.0}),  # rolled rim
    (-0.019, 0.0377, 0.0296, -0.001, 0.0, {"lowerarm": 1.0}),   # inside
    (-0.110, 0.0372, 0.0291, -0.001, 0.0, {"lowerarm": 1.0}),
]
# Finger nails: thin plates laid onto the distal segments (Skin material, rim sunk into the skin).
NAIL_ALONG = (0.32, 0.40, 0.62, 0.80, 0.87)   # fractions of the distal bone
NAIL_ACROSS = (-0.5, -0.36, 0.0, 0.36, 0.5)   # fractions of the nail width
NAIL_WIDTH = 0.0112                           # for size scale 1 (thumb uses THUMB_NAIL_WIDTH)
THUMB_NAIL_WIDTH = 0.0135
NAIL_RAISE, NAIL_RAISE_CORNER, NAIL_SINK = 0.00045, 0.00028, 0.00012
BUTTON_X = -0.047
BUTTON_ANGLE = 162.0  # around the forearm: 0 = thumb side, 90 = back of the wrist, 180 = pinky side
BUTTON_R = 0.0052

# -----------------------------------------------------------------------------
# Final pose (right arm, Blender coordinates relative to the eye; the left arm is mirrored)
# -----------------------------------------------------------------------------

# The shoulder is never on screen; it sits a little further back than anatomical so the forearm comes in
# flatter and the cuff shows at the bottom corners.
SHOULDER_R = Vector((-0.150, -0.190, -0.195))
WRIST_R = Vector((0.330, -0.185, -0.105))
ELBOW_POLE_R = Vector((-0.10, -0.40, -1.00))  # elbows point down and a little outward
HAND_FWD_R = Vector((0.95, 0.22, 0.18))        # wrist -> fingers: forward, inward, slightly up
PALM_NORMAL_R = Vector((0.00, 0.62, -0.78))    # palm faces inward and down

# Relaxed finger flexion (degrees) per finger bone (_01, _02, _03).
FINGER_CURL = {
    "index": (10.0, 18.0, 10.0),
    "middle": (13.0, 23.0, 12.0),
    "ring": (17.0, 27.0, 14.0),
    "pinky": (21.0, 31.0, 16.0),
}
# Thumb local rotations (degrees, X = flexion toward the palm is negative, Y twist, Z swing).
THUMB_POSE = [(-4.0, 0.0, 0.0), (-16.0, 0.0, 0.0), (-28.0, 0.0, 0.0)]


# -----------------------------------------------------------------------------
# Small helpers
# -----------------------------------------------------------------------------


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    args = {"previews": "--previews" in argv, "agent_json": None, "debug_dir": None}
    for key, flag in (("agent_json", "--agent-json"), ("debug_dir", "--debug-dir")):
        if flag in argv:
            args[key] = Path(argv[argv.index(flag) + 1])
    return args


def frame_matrix(head, y_axis, z_hint):
    """Bone-style matrix: Y along the bone, Z toward z_hint (orthogonalized), X = Y x Z."""
    y = Vector(y_axis).normalized()
    z = Vector(z_hint) - y * Vector(z_hint).dot(y)
    z.normalize()
    x = y.cross(z)
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = Vector(head)
    return m


def rot_local(rx, ry=0.0, rz=0.0):
    return Euler((math.radians(rx), math.radians(ry), math.radians(rz)), "XYZ").to_matrix().to_4x4()


def two_bone_ik(shoulder, wrist, l1, l2, pole):
    d = wrist - shoulder
    dist = min(d.length, (l1 + l2) * 0.999)
    dn = d.normalized()
    a = (l1 * l1 + dist * dist - l2 * l2) / (2.0 * dist)
    h = math.sqrt(max(l1 * l1 - a * a, 0.0))
    p = pole - dn * pole.dot(dn)
    p.normalize()
    return shoulder + dn * a + p * h


def hex_offsets(a, b):
    """6-vert finger ring around the finger axis: DL, T, DR, PR, B, PL (L = pinky side, D = back of hand)."""
    return [(-a, 0.5 * b), (0.0, b), (a, 0.5 * b), (a, -0.5 * b), (0.0, -b), (-a, -0.5 * b)]


class Cage:
    def __init__(self):
        self.verts = []
        self.weights = []
        self.faces = []

    def vert(self, co, weights):
        self.verts.append(Vector(co))
        self.weights.append(dict(weights))
        return len(self.verts) - 1

    def ring(self, points, weights):
        if isinstance(weights, dict):
            weights = [weights] * len(points)
        return [self.vert(p, w) for p, w in zip(points, weights)]

    def bridge(self, ra, rb, mat, skip=()):
        n = len(ra)
        for j in range(n):
            if j in skip:
                continue
            k = (j + 1) % n
            self.faces.append(((ra[j], ra[k], rb[k], rb[j]), mat))

    def face(self, idx, mat):
        self.faces.append((tuple(idx), mat))


# -----------------------------------------------------------------------------
# Canonical skeleton (right arm)
# -----------------------------------------------------------------------------


def finger_axes(name):
    y, xm, z, splay, lens, sc = FINGERS[name]
    d = Vector((math.cos(math.radians(splay)), math.sin(math.radians(splay)), 0.0))
    side = Vector((-d.y, d.x, 0.0))  # toward the thumb
    up = Vector((0.0, 0.0, 1.0))
    return Vector((xm, y, z)), d, side, up, lens, sc


def thumb_axes():
    d = THUMB_DIR.normalized()
    z = THUMB_NAIL_HINT - d * THUMB_NAIL_HINT.dot(d)
    z.normalize()
    x = d.cross(z)
    return d, x, z


def canonical_bones():
    """name -> dict(parent, rest (4x4, canonical), length, connect)."""
    bones = {}
    up = Vector((0.0, 0.0, 1.0))
    bones["upperarm"] = dict(parent=None, rest=frame_matrix((-LOWERARM_LEN - UPPERARM_LEN, 0, 0), (1, 0, 0), up),
                             length=UPPERARM_LEN, connect=False)
    bones["lowerarm"] = dict(parent="upperarm", rest=frame_matrix((-LOWERARM_LEN, 0, 0), (1, 0, 0), up),
                             length=LOWERARM_LEN, connect=True)
    bones["hand"] = dict(parent="lowerarm", rest=frame_matrix((0, 0, 0), (1, 0, 0), up), length=HAND_LEN,
                         connect=True)
    td, tx, tz = thumb_axes()
    s = 0.0
    for i, length in enumerate(THUMB_LENS):
        name = f"thumb_0{i + 1}"
        bones[name] = dict(parent="hand" if i == 0 else f"thumb_0{i}", rest=frame_matrix(THUMB_CMC + td * s, td, tz),
                           length=length, connect=i > 0)
        s += length
    for fname in FINGER_ORDER:
        mcp, d, side, upv, lens, sc = finger_axes(fname)
        s = 0.0
        for i, length in enumerate(lens):
            name = f"{fname}_0{i + 1}"
            bones[name] = dict(parent="hand" if i == 0 else f"{fname}_0{i}", rest=frame_matrix(mcp + d * s, d, upv),
                               length=length, connect=i > 0)
            s += length
    return bones


def bone_order(bones):
    order, done = [], set()
    while len(order) < len(bones):
        for name, b in bones.items():
            if name not in done and (b["parent"] is None or b["parent"] in done):
                order.append(name)
                done.add(name)
    return order


def target_matrices(bones):
    """Final (rest = idle) matrices of the right arm bones in Blender space."""
    fwd = HAND_FWD_R.normalized()
    dorsal = -PALM_NORMAL_R
    dorsal = (dorsal - fwd * dorsal.dot(fwd)).normalized()
    thumb_side = dorsal.cross(fwd)
    r_hand = Matrix((fwd, thumb_side, dorsal)).transposed().to_4x4()  # canonical hand space -> world
    place_hand = Matrix.Translation(WRIST_R) @ r_hand

    elbow = two_bone_ik(SHOULDER_R, WRIST_R, UPPERARM_LEN, LOWERARM_LEN, ELBOW_POLE_R)
    fore_dir = (WRIST_R - elbow).normalized()
    upper_dir = (elbow - SHOULDER_R).normalized()

    tgt = {}
    tgt["upperarm"] = frame_matrix(SHOULDER_R, upper_dir, dorsal)
    tgt["lowerarm"] = frame_matrix(elbow, fore_dir, dorsal)
    tgt["hand"] = place_hand @ bones["hand"]["rest"]

    local_pose = {}
    for fname, curl in FINGER_CURL.items():
        for i in range(3):
            local_pose[f"{fname}_0{i + 1}"] = rot_local(-curl[i])
    for i, (rx, ry, rz) in enumerate(THUMB_POSE):
        local_pose[f"thumb_0{i + 1}"] = rot_local(rx, ry, rz)

    for name in bone_order(bones):
        if name in tgt:
            continue
        b = bones[name]
        parent = b["parent"]
        rel = bones[parent]["rest"].inverted() @ b["rest"]
        tgt[name] = tgt[parent] @ rel @ local_pose.get(name, Matrix.Identity(4))
    return tgt


# -----------------------------------------------------------------------------
# Cage: skin (forearm stub, palm, fingers, thumb)
# -----------------------------------------------------------------------------


def superellipse_z(u, n):
    u = min(abs(u), 0.999)
    return (1.0 - u ** n) ** (1.0 / n)


def build_skin(cage):
    # --- knuckle ring from the finger base hexagons
    hexes = {}
    for fname in FINGER_ORDER:
        mcp, d, side, up, lens, sc = finger_axes(fname)
        a, b = FINGER_BASE[0] * sc, FINGER_BASE[1] * sc
        pts = []
        for j, (oy, oz) in enumerate(hex_offsets(a, b)):
            pts.append(mcp + side * (oy * INFL) + up * (oz * INFL) + d * KNUCKLE_DX[j])
        hexes[fname] = pts

    def avg(p, q):
        return (p + q) * 0.5

    P, R, M, I = (hexes[n] for n in FINGER_ORDER)
    D = [P[0], P[1], avg(P[2], R[0]), R[1], avg(R[2], M[0]), M[1], avg(M[2], I[0]), I[1], I[2]]
    Pl = [I[3], I[4], avg(I[5], M[3]), M[4], avg(M[5], R[3]), R[4], avg(R[5], P[3]), P[4], P[5]]
    # Flare the outer edges a little (index / pinky metacarpal heads).
    for p in (D[0], Pl[8]):
        p.y -= 0.0025
    for p in (D[8], Pl[0]):
        p.y += 0.0020
    knuckle_pts = D + Pl

    def knuckle_weights():
        ws = []
        for k in range(9):  # dorsal
            if k % 2 == 1:
                f = FINGER_ORDER[(k - 1) // 2]
                ws.append({"hand": 0.5, f"{f}_01": 0.5})
            else:
                fs = [FINGER_ORDER[i] for i in (k // 2 - 1, k // 2) if 0 <= i < 4]
                w = {"hand": 0.5}
                for f in fs:
                    w[f"{f}_01"] = 0.5 / len(fs)
                ws.append(w)
        dorsal = ws
        palmar = []
        for k in range(9):  # P_k mirrors D_(8-k)
            palmar.append(dorsal[8 - k])
        return dorsal + palmar

    # --- forearm + palm rings (18 verts: D0..D8 back of hand pinky->thumb, P0..P8 palm thumb->pinky)
    rings = []
    for x, hw, ht, hb, zc in FOREARM_RINGS:
        pts = []
        for k in range(9):
            th = math.radians(170.0 - 20.0 * k)
            pts.append(Vector((x, hw * math.cos(th) * INFL, zc + ht * math.sin(th) * INFL)))
        for k in range(9):
            th = math.radians(-10.0 - 20.0 * k)
            pts.append(Vector((x, hw * math.cos(th) * INFL, zc + hb * math.sin(th) * INFL)))
        rings.append(pts)

    Dy = [p.y for p in D]
    Py = [p.y for p in Pl]
    span = max(-min(Dy + Py), max(Dy + Py))
    for pr, (x, hw, ht, hb, zc, n, thenar, hypo) in enumerate(PALM_RINGS):
        pts = []
        for k in range(9):
            y = Dy[k] / span * hw
            u = y / hw
            z = 0.40 * ht if k in (0, 8) else ht * superellipse_z(u, n) * (1.0 - 0.10 * u * u)
            pts.append(Vector((x, y * INFL, zc + z * INFL)))
        for k in range(9):
            y = Py[k] / span * hw
            u = y / hw
            if k == 8:
                z = -0.40 * hb
            elif k == 0:
                # radial wall; raised where the thumb opening is so the opening faces radial-palmar
                z = -0.20 * hb if pr in THUMB_PALM_RINGS else -0.40 * hb
            else:
                z = -hb * superellipse_z(u, n)
                z -= thenar * hb * max(0.0, u - 0.15) / 0.85
                z -= hypo * hb * max(0.0, -u - 0.35) / 0.65
            pts.append(Vector((x, y * INFL, zc + z * INFL)))
        rings.append(pts)
    rings.append(knuckle_pts)

    n_fore = len(FOREARM_RINGS)
    ring_w = [{"lowerarm": 1.0}] * (n_fore - 1) + [{"lowerarm": 0.85, "hand": 0.15}]
    ring_w += [{"lowerarm": 0.35, "hand": 0.65}] + [{"hand": 1.0}] * (len(PALM_RINGS) - 1) + [None]
    hole_rings = [n_fore + pr for pr in THUMB_PALM_RINGS]
    ring_idx = []
    for r, pts in enumerate(rings):
        if ring_w[r] is None:
            ws = knuckle_weights()
        else:
            ws = [dict(ring_w[r]) for _ in pts]
            if r in hole_rings:  # thumb opening border verts (P0, P1)
                for k in (9, 10):
                    ws[k] = {"hand": 0.8, "thumb_01": 0.2}
        ring_idx.append(cage.ring(pts, ws))

    cage.face(list(reversed(ring_idx[0])), I_SKIN)
    # The palm quads between P0 and P1 (radial-palmar corner) are left open for the thumb.
    for r in range(len(ring_idx) - 1):
        skip = (9,) if (r in hole_rings and r + 1 in hole_rings) else ()
        cage.bridge(ring_idx[r], ring_idx[r + 1], I_SKIN, skip=skip)

    # --- fingers
    K = ring_idx[-1]
    for fi, fname in enumerate(FINGER_ORDER):
        mcp, d, side, up, lens, sc = finger_axes(fname)
        base = [K[2 * fi], K[2 * fi + 1], K[2 * fi + 2], K[9 + 6 - 2 * fi], K[9 + 7 - 2 * fi], K[9 + 8 - 2 * fi]]
        prev = base
        for ph, frac, a, b, dz in FINGER_STATIONS:
            s = sum(lens[:ph]) + frac * lens[ph]
            c = mcp + d * s + up * dz
            pts = [c + side * (oy * sc * INFL) + up * (oz * sc * INFL) for oy, oz in hex_offsets(a, b)]
            if frac == 1.0:  # knuckle bump on the back of the finger
                pts[1] += up * 0.0007
            if frac == 1.0 and ph < 2:
                w = {f"{fname}_0{ph + 1}": 0.5, f"{fname}_0{ph + 2}": 0.5}
            else:
                w = {f"{fname}_0{ph + 1}": 1.0}
            cur = cage.ring(pts, w)
            cage.bridge(prev, cur, I_SKIN)
            prev = cur
        cage.face([prev[0], prev[1], prev[2], prev[3]], I_SKIN)
        cage.face([prev[3], prev[4], prev[5], prev[0]], I_SKIN)

    # --- thumb
    R1, R2, R3 = (ring_idx[r] for r in hole_rings)
    base = [R1[9], R2[9], R3[9], R3[10], R2[10], R1[10]]
    td, tx, tz = thumb_axes()
    c0 = sum((cage.verts[i] for i in base), Vector()) / len(base)
    raw = []
    for i in base:
        v = cage.verts[i] - c0
        raw.append(math.atan2(v.dot(tz), v.dot(tx)))
    # unwrap and regularize to an even hexagon in the same winding
    un = [raw[0]]
    for a in raw[1:]:
        while a - un[-1] > math.pi:
            a -= 2 * math.pi
        while a - un[-1] < -math.pi:
            a += 2 * math.pi
        un.append(a)
    sgn = 1.0 if un[-1] > un[0] else -1.0
    angles = [0.5 * un[j] + 0.5 * (un[0] + sgn * j * math.pi / 3.0) for j in range(6)]
    prev = base
    for si, (ph, frac, a, b) in enumerate(THUMB_STATIONS):
        s = sum(THUMB_LENS[:ph]) + frac * THUMB_LENS[ph]
        c = THUMB_CMC + td * s
        if si == 0:  # thenar ring sits between the palm opening and the bone axis
            c = c0.lerp(c, THUMB_T1_BLEND)
        pts = [c + tx * (a * math.cos(t) * INFL) + tz * (b * math.sin(t) * INFL) for t in angles]
        if ph == 0 and frac < 1.0:
            w = {"thumb_01": 0.75, "hand": 0.25}
        elif frac == 1.0 and ph < 2:
            w = {f"thumb_0{ph + 1}": 0.5, f"thumb_0{ph + 2}": 0.5}
        else:
            w = {f"thumb_0{ph + 1}": 1.0}
        cur = cage.ring(pts, w)
        cage.bridge(prev, cur, I_SKIN)
        prev = cur
    cage.face([prev[0], prev[1], prev[2], prev[3]], I_SKIN)
    cage.face([prev[3], prev[4], prev[5], prev[0]], I_SKIN)


# -----------------------------------------------------------------------------
# Cage: sleeve with cuff
# -----------------------------------------------------------------------------


def build_sleeve(cage):
    idx = []
    for r, (x, hw, hh, zc, fold, w) in enumerate(SLEEVE_RINGS):
        pts = []
        for j in range(SLEEVE_N):
            th = 2.0 * math.pi * (j + 0.5) / SLEEVE_N
            m = 1.0 + fold * math.sin(3.0 * th + 1.7 * r) + 0.5 * fold * math.sin(5.0 * th + 0.9 * r + 0.4)
            m *= SLEEVE_INFL
            pts.append(Vector((x + 0.004 * fold / 0.05 * math.sin(2.0 * th + r), hw * math.cos(th) * m,
                               zc + hh * math.sin(th) * m)))
        idx.append(cage.ring(pts, w))
    cage.face(list(reversed(idx[0])), I_SLEEVE)
    for r in range(len(idx) - 1):
        cage.bridge(idx[r], idx[r + 1], I_SLEEVE)
    cage.face(idx[-1], I_SLEEVE)


# -----------------------------------------------------------------------------
# Mesh assembly
# -----------------------------------------------------------------------------


def subdivide_cage(cage):
    """Create the cage as a temporary object, apply one Catmull-Clark level, read the result back."""
    bm = bmesh.new()
    bverts = [bm.verts.new(co) for co in cage.verts]
    for idx, mat in cage.faces:
        f = bm.faces.new([bverts[i] for i in idx])
        f.material_index = mat
    bm.normal_update()
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new("_cage")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("_cage", mesh)
    bpy.context.scene.collection.objects.link(obj)
    groups = {}
    for i, w in enumerate(cage.weights):
        for name, value in w.items():
            if name not in groups:
                groups[name] = obj.vertex_groups.new(name=name)
            groups[name].add([i], value, "REPLACE")
    mod = obj.modifiers.new("Subdivision", "SUBSURF")
    mod.levels = 1
    mod.render_levels = 1
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    names = {g.index: g.name for g in obj.vertex_groups}
    verts = [v.co.copy() for v in mesh.vertices]
    weights = []
    for v in mesh.vertices:
        w = {names[g.group]: g.weight for g in v.groups if g.weight > 1e-4}
        total = sum(w.values())
        weights.append({k: x / total for k, x in w.items()})
    faces = [(tuple(p.vertices), p.material_index) for p in mesh.polygons]
    bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.meshes.remove(mesh)
    return verts, faces, weights


def add_button(verts, faces, weights):
    """Small dark cuff button on the pinky/back side of the cuff (rigid on the forearm)."""
    th = math.radians(BUTTON_ANGLE)
    direction = Vector((0.0, math.cos(th), math.sin(th)))
    best, best_score = None, -1e9
    for i, co in enumerate(verts):
        if abs(co.x - BUTTON_X) > 0.006 or weights[i].get("lowerarm", 0.0) < 0.99:
            continue
        radial = Vector((0.0, co.y, co.z + 0.001))
        if radial.length < 0.035:
            continue
        score = radial.normalized().dot(direction) - abs(co.x - BUTTON_X) * 5.0
        if score > best_score:
            best, best_score = co, score
    centre_r = Vector((0.0, best.y, best.z + 0.001)).length
    normal = direction
    centre = Vector((BUTTON_X, 0.0, -0.001)) + normal * (centre_r + 0.0004)
    bm = rh.cylinder(BUTTON_R, 0.0022, (0.0, 0.0, 0.0), axis="Z", mat_index=I_BUTTON, segments=8)
    top = [v for v in bm.verts if v.co.z > 0]
    for v in top:
        v.co.x *= 0.82
        v.co.y *= 0.82
    rot = Vector((0, 0, 1)).rotation_difference(normal).to_matrix().to_4x4()
    bmesh.ops.transform(bm, matrix=Matrix.Translation(centre) @ rot, verts=bm.verts)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    base = len(verts)
    for v in bm.verts:
        verts.append(v.co.copy())
        weights.append({"lowerarm": 1.0})
    bm.verts.index_update()
    for f in bm.faces:
        faces.append((tuple(base + v.index for v in f.verts), I_BUTTON))
    bm.free()


def add_nails(verts, faces, weights):
    """Thin nail plates on the back of each distal segment, found by ray casting onto the skin (canonical pose).
    The outer ring of each plate is sunk into the skin and the inner part raised, so a small nail rim catches
    the light without a separate material."""
    from mathutils.bvhtree import BVHTree

    bvh = BVHTree.FromPolygons(verts, [idx for idx, mat in faces if mat == I_SKIN])
    specs = []
    for fname in FINGER_ORDER:
        mcp, d, side, up, lens, sc = finger_axes(fname)
        specs.append((f"{fname}_03", mcp + d * (lens[0] + lens[1]), d, side, up, lens[2], NAIL_WIDTH * sc))
    td, tx, tz = thumb_axes()
    specs.append(("thumb_03", THUMB_CMC + td * (THUMB_LENS[0] + THUMB_LENS[1]), td, tx, tz, THUMB_LENS[2],
                  THUMB_NAIL_WIDTH))
    na, nc = len(NAIL_ALONG), len(NAIL_ACROSS)
    for bone, base, d, side, up, length, width in specs:
        grid = []
        for i, u in enumerate(NAIL_ALONG):
            for j, v in enumerate(NAIL_ACROSS):
                hit = None
                for shrink in (1.0, 0.9, 0.8, 0.7, 0.6):  # narrow toward the rounded fingertip if needed
                    origin = base + d * (u * length) + side * (v * width * shrink) + up * 0.03
                    hit, normal, _index, _dist = bvh.ray_cast(origin, -up, 0.06)
                    if hit is not None and normal.dot(up) > 0.2:
                        break
                    hit = None
                if hit is None:
                    break
                rim = i in (0, na - 1) or j in (0, nc - 1)
                corner = i in (1, na - 2) and j in (1, nc - 2)
                lift = -NAIL_SINK if rim else (NAIL_RAISE_CORNER if corner else NAIL_RAISE)
                grid.append(hit + normal * lift)
            else:
                continue
            break
        if len(grid) != na * nc:
            print(f"WARNING: nail for {bone} skipped (ray miss)")
            continue
        start = len(verts)
        for co in grid:
            verts.append(co)
            weights.append({bone: 1.0})
        for i in range(na - 1):
            for j in range(nc - 1):
                q = [start + i * nc + j, start + (i + 1) * nc + j, start + (i + 1) * nc + j + 1, start + i * nc + j + 1]
                n = (verts[q[1]] - verts[q[0]]).cross(verts[q[3]] - verts[q[0]])
                if n.dot(up) < 0.0:
                    q.reverse()
                faces.append((tuple(q), I_SKIN))


def deform(verts, weights, bones, tgt):
    skin = {name: tgt[name] @ b["rest"].inverted() for name, b in bones.items()}
    out = []
    for co, w in zip(verts, weights):
        p = Vector((0.0, 0.0, 0.0))
        for name, value in w.items():
            p += (skin[name] @ co) * value
        out.append(p)
    return out


def build_mesh(materials, bones, tgt):
    cage = Cage()
    build_skin(cage)
    build_sleeve(cage)
    verts, faces, weights = subdivide_cage(cage)
    add_button(verts, faces, weights)
    add_nails(verts, faces, weights)
    verts = deform(verts, weights, bones, tgt)

    # Right arm (Blender -Y, bones _r) + mirrored left arm (Blender +Y, bones _l).
    all_verts, all_faces, all_weights = [], [], []
    for side in ("r", "l"):
        base = len(all_verts)
        sy = -1.0 if side == "l" else 1.0
        for co in verts:
            all_verts.append(Vector((co.x, co.y * sy, co.z)))
        for w in weights:
            all_weights.append({f"{k}_{side}": v for k, v in w.items()})
        for idx, mat in faces:
            idx = tuple(base + i for i in idx)
            if side == "l":  # flip winding but keep the first corner so quads triangulate the same way
                idx = (idx[0],) + tuple(reversed(idx[1:]))
            all_faces.append((idx, mat))

    mesh = bpy.data.meshes.new(MESH_NAME + "_Mesh")
    mesh.from_pydata([tuple(v) for v in all_verts], [], [f for f, _ in all_faces])
    mesh.polygons.foreach_set("material_index", [m for _, m in all_faces])
    for mat in materials:
        mesh.materials.append(mat)
    mesh.shade_smooth()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    rh.box_project_uvs(bm, 10.0)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    obj = bpy.data.objects.new(MESH_NAME, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj, all_weights


# -----------------------------------------------------------------------------
# Armature + skinning
# -----------------------------------------------------------------------------


def side_bone_specs(bones, tgt, side):
    specs = []
    sy = -1.0 if side == "l" else 1.0

    def mirror(v):
        return Vector((v.x, v.y * sy, v.z))

    for name in bone_order(bones):
        b = bones[name]
        m = tgt[name]
        head = mirror(m.translation)
        y = mirror(m.col[1].xyz)
        z = mirror(m.col[2].xyz)
        parent = f"{b['parent']}_{side}" if b["parent"] else "root"
        specs.append((f"{name}_{side}", parent, head, head + y * b["length"], z, b["connect"]))
    return specs


def create_armature(bones, tgt):
    arm_data = bpy.data.armatures.new(ARMATURE_NAME)
    arm_data.display_type = "OCTAHEDRAL"
    arm_obj = bpy.data.objects.new(ARMATURE_NAME, arm_data)
    bpy.context.scene.collection.objects.link(arm_obj)
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.object.mode_set(mode="EDIT")
    ebs = {}
    root = arm_data.edit_bones.new("root")
    root.head = (0.0, 0.0, 0.0)
    root.tail = (0.0, 0.10, 0.0)
    root.roll = 0.0
    root.use_deform = False
    ebs["root"] = root
    for side in ("r", "l"):
        for name, parent, head, tail, z, connect in side_bone_specs(bones, tgt, side):
            eb = arm_data.edit_bones.new(name)
            eb.head = head
            eb.tail = tail
            eb.align_roll(z)
            eb.parent = ebs[parent]
            eb.use_connect = connect
            eb.use_deform = True
            ebs[name] = eb
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in arm_obj.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return arm_obj


def skin_mesh(mesh_obj, arm_obj, weights):
    groups = {}
    for b in arm_obj.data.bones:
        if b.use_deform:
            groups[b.name] = mesh_obj.vertex_groups.new(name=b.name)
    for i, w in enumerate(weights):
        for name, value in w.items():
            groups[name].add([i], value, "REPLACE")
    mesh_obj.parent = arm_obj
    mod = mesh_obj.modifiers.new("Armature", "ARMATURE")
    mod.object = arm_obj
    mod.use_vertex_groups = True
    mod.use_bone_envelopes = False


# -----------------------------------------------------------------------------
# Animation
# -----------------------------------------------------------------------------


FINGER_ANIM_ORDER = ["index", "middle", "ring", "pinky"]


def local_q(side, rx, ry=0.0, rz=0.0):
    """Local bone rotation in degrees; Y/Z flip on the left side so both sides move mirror-symmetrically."""
    s = 1.0 if side == "r" else -1.0
    return Euler((math.radians(rx), math.radians(ry * s), math.radians(rz * s)), "XYZ").to_quaternion()


def arm_offsets(arm_obj, side, d_world, rot_world):
    """Upperarm pose (location, rotation) for a world-space shift and rotation of the whole arm."""
    rest = arm_obj.data.bones[f"upperarm_{side}"].matrix_local.to_quaternion()
    loc = rest.inverted() @ d_world
    q = rest.inverted() @ rot_world @ rest
    return loc, q


def arm_rotation(arm_obj, side, lift_deg, inward_deg):
    """Small rotation about the shoulder that lifts / swings the hand inward."""
    s_head = arm_obj.data.bones[f"upperarm_{side}"].head_local
    h_head = arm_obj.data.bones[f"hand_{side}"].head_local
    r = h_head - s_head
    k_up = r.cross(Vector((0, 0, 1))).normalized()
    inward = Vector((0.0, 1.0 if side == "r" else -1.0, 0.0))
    k_in = r.cross(inward).normalized()
    return Quaternion(k_up, math.radians(lift_deg)) @ Quaternion(k_in, math.radians(inward_deg))


def idle_pose(arm_obj, t):
    ph = 2.0 * math.pi * t
    pose = {}
    for side, off, amp in (("r", 0.0, 1.0), ("l", 0.55, 0.85)):
        p = ph + off
        inward = 1.0 if side == "r" else -1.0
        d = Vector((0.0020 * math.sin(p + 1.3),
                    0.0010 * inward * math.sin(p + 2.1),
                    0.0042 * amp * math.sin(p) + 0.0012 * math.sin(2.0 * p + 0.7)))
        rot = arm_rotation(arm_obj, side, 0.22 * math.sin(p + 0.4), 0.20 * math.sin(p + 1.9))
        loc, q = arm_offsets(arm_obj, side, d, rot)
        pose[f"upperarm_{side}"] = (loc, q)
        pose[f"lowerarm_{side}"] = (None, local_q(side, 0.4 * math.sin(p + 2.4)))
        pose[f"hand_{side}"] = (None, local_q(side, 1.6 * math.sin(p + 0.5) + 0.5 * math.sin(2.0 * p + 1.1),
                                              1.0 * math.sin(p + 1.7), 0.8 * math.sin(p + 2.6)))
        for i, fname in enumerate(FINGER_ANIM_ORDER):
            c = 2.6 * (0.5 + 0.5 * math.sin(p - 0.6 - 0.5 * i + (0.3 if side == "l" else 0.0)))
            for j, k in enumerate((0.8, 1.0, 0.6)):
                pose[f"{fname}_0{j + 1}_{side}"] = (None, local_q(side, -k * c))
        tc = 1.5 * math.sin(p - 0.3)
        pose[f"thumb_01_{side}"] = (None, local_q(side, 0.0, 0.0, 0.6 * math.sin(p + 0.8)))
        pose[f"thumb_02_{side}"] = (None, local_q(side, -0.6 * tc))
        pose[f"thumb_03_{side}"] = (None, local_q(side, -tc))
    return pose


def walk_pose(arm_obj, t):
    ph = 2.0 * math.pi * t
    pose = {}
    for side, alt in (("r", 1.0), ("l", -1.0)):
        inward = 1.0 if side == "r" else -1.0
        d = Vector((0.0055 * alt * math.cos(ph),
                    0.0025 * inward * alt * math.sin(ph),
                    0.0050 * alt * math.sin(ph + 0.5) - 0.0028 * math.cos(2.0 * ph)))
        rot = arm_rotation(arm_obj, side, 0.45 * alt * math.sin(ph + 0.9), 0.45 * alt * math.cos(ph))
        loc, q = arm_offsets(arm_obj, side, d, rot)
        pose[f"upperarm_{side}"] = (loc, q)
        pose[f"lowerarm_{side}"] = (None, local_q(side, 1.2 * alt * math.sin(ph + 1.2)))
        pose[f"hand_{side}"] = (None, local_q(side, 2.0 * alt * math.sin(ph + 1.0) + 0.8 * math.cos(2.0 * ph + 0.4),
                                              1.5 * alt * math.sin(ph + 0.3), 1.0 * alt * math.sin(ph + 1.4)))
        for i, fname in enumerate(FINGER_ANIM_ORDER):
            c = 2.2 * (0.5 + 0.5 * alt * math.sin(ph + 1.6 - 0.35 * i))
            for j, k in enumerate((0.8, 1.0, 0.6)):
                pose[f"{fname}_0{j + 1}_{side}"] = (None, local_q(side, -k * c))
        tc = 1.5 * alt * math.sin(ph + 1.2)
        pose[f"thumb_01_{side}"] = (None, local_q(side, 0.0))
        pose[f"thumb_02_{side}"] = (None, local_q(side, -0.6 * tc))
        pose[f"thumb_03_{side}"] = (None, local_q(side, -tc))
    return pose


def make_actions(arm_obj):
    bpy.context.preferences.edit.keyframe_new_interpolation_type = "LINEAR"
    arm_obj.animation_data_create()
    actions = {}
    for name, fn in (("Idle", idle_pose), ("Walk", walk_pose)):
        nframes = ACTION_FRAMES[name]
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        arm_obj.animation_data.action = act
        for f in range(nframes + 1):
            pose = fn(arm_obj, (f % nframes) / nframes)  # last frame == first frame exactly
            for pb in arm_obj.pose.bones:
                loc, q = pose.get(pb.name, (None, Quaternion()))
                pb.location = loc if loc is not None else Vector()
                pb.rotation_quaternion = q
                pb.keyframe_insert("rotation_quaternion", frame=f, group=pb.name)
                if pb.name.startswith("upperarm_"):
                    pb.keyframe_insert("location", frame=f, group=pb.name)
        act.use_frame_range = True
        act.frame_start = 0
        act.frame_end = nframes
        act.use_cyclic = True
        actions[name] = act
    reset_pose(arm_obj)
    return actions


def reset_pose(arm_obj):
    for pb in arm_obj.pose.bones:
        pb.location = Vector()
        pb.rotation_quaternion = Quaternion()
        pb.scale = Vector((1, 1, 1))


def set_action(arm_obj, act):
    arm_obj.animation_data.action = act
    if act is not None and arm_obj.animation_data.action_slot is None and len(act.slots):
        arm_obj.animation_data.action_slot = act.slots[0]


# -----------------------------------------------------------------------------
# Scene / export
# -----------------------------------------------------------------------------


def make_materials():
    mats = []
    for name, color, metallic, roughness, _family, _grime in MATERIALS:
        mats.append(rh.make_material(name, color, metallic, roughness))
    return mats


def set_scene():
    scene = bpy.context.scene
    rh.set_scene(
        AssetPurpose="Self-made first-person arms (office worker shirt sleeves + hands) for RulesHorror.",
        CameraSpace="Origin = eye point of the first-person camera. Blender +X = view forward, -Y = right, +Z = up.",
        ExportFBX=str(OUTPUT_FBX),
    )
    scene.render.fps = FPS
    scene.render.fps_base = 1.0
    scene.frame_start = 0
    scene.frame_end = ACTION_FRAMES["Idle"]


def mesh_stats(obj):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    xs = [v.co.x * 100 for v in obj.data.vertices]
    ys = [-v.co.y * 100 for v in obj.data.vertices]
    zs = [v.co.z * 100 for v in obj.data.vertices]
    bounds = {"min": [round(min(xs), 2), round(min(ys), 2), round(min(zs), 2)],
              "max": [round(max(xs), 2), round(max(ys), 2), round(max(zs), 2)]}
    return tris, bounds


def scale_rig(arm_obj, mesh_obj, k):
    """Scale the rig data by k: mesh vertices, bone rest positions and every action's location keys.
    Object transforms stay identity (the mesh is parented to the armature with an identity parent inverse)."""
    mesh_obj.data.transform(Matrix.Scale(k, 4))
    arm_obj.data.transform(Matrix.Scale(k, 4))
    for act in bpy.data.actions:
        for layer in act.layers:
            for strip in layer.strips:
                for bag in strip.channelbags:
                    for fc in bag.fcurves:
                        if fc.data_path.endswith(".location"):
                            for kp in fc.keyframe_points:
                                kp.co.y *= k
                                kp.handle_left.y *= k
                                kp.handle_right.y *= k
                            fc.update()


def export_fbx(arm_obj, mesh_obj):
    """Export in centimeters so no FBX node carries a scale.
    With the model in meters the exporter puts a x100 scale on the Armature node; Unreal then imported either
    a 100x too small mesh (FBX_SCALE_ALL) or a correct bind pose whose animation tracks did not match it, so the
    arms vanished as soon as an animation played (FBX_SCALE_NONE). The scene is restored after the export."""
    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    scene = bpy.context.scene
    set_action(arm_obj, None)
    reset_pose(arm_obj)
    scene.frame_set(0)
    old_scale_length = scene.unit_settings.scale_length
    scale_rig(arm_obj, mesh_obj, 100.0)
    scene.unit_settings.scale_length = 0.01  # 1 Blender unit = 1 cm -> unit scale factor 1 in the FBX
    try:
        bpy.ops.object.select_all(action="DESELECT")
        arm_obj.select_set(True)
        mesh_obj.select_set(True)
        bpy.context.view_layer.objects.active = arm_obj
        bpy.ops.export_scene.fbx(
            filepath=str(OUTPUT_FBX), use_selection=True, object_types={"ARMATURE", "MESH"},
            add_leaf_bones=False, bake_anim=True, bake_anim_use_all_actions=True, bake_anim_use_nla_strips=False,
            bake_anim_force_startend_keying=True, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
            global_scale=1.0, axis_forward="-Z", axis_up="Y", primary_bone_axis="Y", secondary_bone_axis="X",
            mesh_smooth_type="FACE", use_armature_deform_only=True, use_mesh_modifiers=True)
    finally:
        scene.unit_settings.scale_length = old_scale_length
        scale_rig(arm_obj, mesh_obj, 0.01)
        scene.frame_set(0)


def cleanup_blend1():
    for p in BASE_DIR.glob("*.blend1"):
        p.unlink()


# -----------------------------------------------------------------------------
# Previews (run after saving/exporting; nothing is saved afterwards)
# -----------------------------------------------------------------------------


def setup_preview_render(res_x, res_y):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_cavity = False
    scene.display.shading.show_shadows = False
    scene.display.shading.show_specular_highlight = True
    if scene.world is None:
        scene.world = bpy.data.worlds.new("_PreviewWorld")
    scene.world.color = (0.035, 0.037, 0.04)
    scene.render.resolution_x = res_x
    scene.render.resolution_y = res_y
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.view_settings.view_transform = "Standard"


def make_camera(name, location, rotation=None, look_at=None, fov_deg=70.0):
    cam_data = bpy.data.cameras.new(name)
    cam_data.sensor_fit = "HORIZONTAL"
    cam_data.angle = math.radians(fov_deg)
    cam_data.clip_start = 0.01
    cam_data.clip_end = 50.0
    cam = bpy.data.objects.new(name, cam_data)
    bpy.context.scene.collection.objects.link(cam)
    cam.location = location
    if look_at is not None:
        direction = Vector(look_at) - Vector(location)
        cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    else:
        cam.rotation_euler = rotation
    return cam


def render_to(cam, path):
    scene = bpy.context.scene
    scene.camera = cam
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    return path


def draw_guides(src, dst):
    """Copy of a first-person render with thirds lines (debug only)."""
    import numpy as np
    img = bpy.data.images.load(str(src))
    w, h = img.size
    px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
    for x in (w // 3, 2 * w // 3):
        px[:, x, :3] = (1.0, 0.2, 0.2)
    for y in (h // 3, 2 * h // 3):
        px[y, :, :3] = (1.0, 0.2, 0.2)
    out = bpy.data.images.new("_guides", w, h)
    out.pixels = px.ravel().tolist()
    rh.save_image(out, dst)
    bpy.data.images.remove(img)
    bpy.data.images.remove(out)


def render_previews(arm_obj, mesh_obj, actions, debug_dir=None):
    scene = bpy.context.scene
    setup_preview_render(1280, 720)
    fp_cam = make_camera("_FPCam", (0.0, 0.0, 0.0), rotation=(math.radians(90.0), 0.0, math.radians(-90.0)))
    shots = []
    set_action(arm_obj, None)
    reset_pose(arm_obj)
    scene.frame_set(0)
    shots.append(render_to(fp_cam, PREVIEW_DIR / "FP_Rest.png"))
    set_action(arm_obj, actions["Idle"])
    scene.frame_set(60)
    shots.append(render_to(fp_cam, PREVIEW_DIR / "FP_Idle_f060.png"))
    set_action(arm_obj, actions["Walk"])
    scene.frame_set(15)
    shots.append(render_to(fp_cam, PREVIEW_DIR / "FP_Walk_f015.png"))

    set_action(arm_obj, None)
    reset_pose(arm_obj)
    scene.frame_set(0)
    # 3/4 outside view with a small eye marker at the camera origin.
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.012, location=(0.0, 0.0, 0.0), segments=16, ring_count=8)
    marker = bpy.context.active_object
    marker.name = "_EyeMarker"
    out_cam = make_camera("_OutCam", (0.95, -0.95, 0.45), look_at=(0.18, 0.0, -0.16), fov_deg=40.0)
    shots.append(render_to(out_cam, PREVIEW_DIR / "Outside_34.png"))
    if debug_dir is not None:
        debug_dir = Path(debug_dir)
        draw_guides(PREVIEW_DIR / "FP_Rest.png", debug_dir / "dbg_fp_rest_guides.png")
        # Tint the thumb so it can be told apart in the debug shots (nothing is saved after this).
        dbg = rh.make_material("_DbgThumb", (0.6, 0.15, 0.1, 1.0), 0.0, 0.5)
        mesh_obj.data.materials.append(dbg)
        groups = {g.index: g.name for g in mesh_obj.vertex_groups}
        thumbish = set()
        for v in mesh_obj.data.vertices:
            if sum(g.weight for g in v.groups if groups[g.group].startswith("thumb_0")) > 0.5:
                thumbish.add(v.index)
        for p in mesh_obj.data.polygons:
            if all(i in thumbish for i in p.vertices):
                p.material_index = len(mesh_obj.data.materials) - 1
        hand = arm_obj.data.bones["hand_r"]
        hc = (hand.head_local + hand.tail_local) * 0.5
        tip = arm_obj.data.bones["middle_03_r"]
        nail_look = tip.head_local
        nail_eye = nail_look + tip.matrix_local.col[2].xyz * 0.10 - tip.matrix_local.col[1].xyz * 0.04
        for name, loc, look, fov in (
            ("dbg_fp_zoom_r", (0.0, 0.0, 0.0), tuple(hc), 30.0),
            ("dbg_hand_top", (0.42, -0.20, 0.20), tuple(hc), 35.0),
            ("dbg_hand_palm", (0.48, 0.10, -0.32), tuple(hc), 35.0),
            ("dbg_hand_side", (0.40, -0.55, -0.10), tuple(hc), 35.0),
            ("dbg_hand_front", (0.85, -0.12, -0.05), tuple(hc), 30.0),
            ("dbg_front_both", (1.2, 0.0, -0.05), (0.35, 0.0, -0.14), 40.0),
            ("dbg_nails", tuple(nail_eye), tuple(nail_look), 22.0),
        ):
            marker.hide_render = name.startswith("dbg_fp")
            cam = make_camera("_Dbg_" + name, loc, look_at=look, fov_deg=fov)
            render_to(cam, debug_dir / f"{name}.png")
    return shots


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def main():
    args = parse_args()
    rh.clear_scene()
    for act in list(bpy.data.actions):
        bpy.data.actions.remove(act)
    for arm in list(bpy.data.armatures):
        bpy.data.armatures.remove(arm)
    set_scene()
    materials = make_materials()

    bones = canonical_bones()
    tgt = target_matrices(bones)
    mesh_obj, weights = build_mesh(materials, bones, tgt)
    arm_obj = create_armature(bones, tgt)
    skin_mesh(mesh_obj, arm_obj, weights)
    actions = make_actions(arm_obj)

    tris, bounds = mesh_stats(mesh_obj)
    print(f"{MESH_NAME}: tris={tris} verts={len(mesh_obj.data.vertices)} UE bounds cm min={bounds['min']} "
          f"max={bounds['max']} slots={[m.name for m in mesh_obj.data.materials]}")
    print(f"Armature: bones={len(arm_obj.data.bones)} actions="
          f"{ {n: tuple(a.frame_range) for n, a in actions.items()} }")
    for side in ("r", "l"):
        for bname in (f"upperarm_{side}", f"lowerarm_{side}", f"hand_{side}", f"middle_03_{side}"):
            b = arm_obj.data.bones[bname]
            h, t = b.head_local, b.tail_local
            print(f"  {bname}: head UE cm ({h.x*100:.1f}, {-h.y*100:.1f}, {h.z*100:.1f}) "
                  f"tail ({t.x*100:.1f}, {-t.y*100:.1f}, {t.z*100:.1f})")

    # Save with Idle assigned for convenience, then export from the rest pose.
    set_action(arm_obj, actions["Idle"])
    bpy.context.scene.frame_set(0)
    bpy.ops.object.select_all(action="DESELECT")
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    export_fbx(arm_obj, mesh_obj)
    cleanup_blend1()
    print(f"Saved {OUTPUT_BLEND}\nExported {OUTPUT_FBX}")

    if args["agent_json"] is not None:
        spec = {
            "name": MESH_NAME, "folder": "FPArms", "fbx": OUTPUT_FBX.as_posix(),
            "ue_bounds_cm": bounds, "tris": tris, "bones": len(arm_obj.data.bones),
            "actions": {n: [int(a.frame_range[0]), int(a.frame_range[1])] for n, a in actions.items()},
            "fps": FPS,
            "slots": [{"slot": name, "family": family, "base_color": [round(c, 3) for c in color[:3]],
                       "metallic": metallic, "roughness": roughness, "grime": grime,
                       "mi_name": f"MI_FPArms_{name}"}
                      for name, color, metallic, roughness, family, grime in MATERIALS],
        }
        args["agent_json"].parent.mkdir(parents=True, exist_ok=True)
        args["agent_json"].write_text(json.dumps(spec, indent=2), encoding="utf-8")
        print(f"Wrote {args['agent_json']}")

    if args["previews"]:
        for shot in render_previews(arm_obj, mesh_obj, actions, args["debug_dir"]):
            print(f"Preview {shot}")


if __name__ == "__main__":
    main()
