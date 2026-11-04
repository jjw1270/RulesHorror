"""RulesHorror UE-ready round office wall clock generator (body + three hands).

Builds four meshes in one .blend and exports one FBX per mesh:
  SM_WallClock            replaces MannDev Clock/Merged/SM_Merged_Clock          (body: wood ring, dial, numerals, glass)
  SM_WallClock_HourHand   replaces MannDev Clock/SeperatedMesh/SM_Object_15
  SM_WallClock_MinuteHand replaces MannDev Clock/SeperatedMesh/SM_Object_17
  SM_WallClock_SecondHand replaces MannDev Clock/SeperatedMesh/SM_Object_21

Contract with the C++ clock actor (AClock, used by /Game/Objects/Clock_BP):
  - All four meshes share one pivot: the clock center, at the origin.
  - The clock front (dial, glass) faces Unreal +Y (= Blender -Y); the back (wall side) is Unreal -Y.
  - Each hand points to Unreal +Z (12 o'clock) at rotation 0. C++ turns the hand components with
    FRotator(pitch, 0, 0), pitch = -30 deg/hour, -6 deg/min, -6 deg/sec; negative pitch turns a hand
    toward Unreal +X, which is 3 o'clock as seen from the front.
  - Hands live in separate Unreal Y layers (hour at the back, second at the front) so they never intersect.

Model coordinates below are written as Unreal (x, y, z) in cm and converted with ue().

Run:
  blender --background --factory-startup --python create_wall_clock_blend.py
  (optional) ... --python create_wall_clock_blend.py -- --preview <dir>
"""

import json
import math
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "_Common"))
import rh_prop_utils as rh  # noqa: E402

BASE_DIR = Path(__file__).resolve().parent
OUTPUT_BLEND = BASE_DIR / "WallClock.blend"
EXPORT_DIR = BASE_DIR / "Exports"

BODY_NAME = "SM_WallClock"
HOUR_NAME = "SM_WallClock_HourHand"
MINUTE_NAME = "SM_WallClock_MinuteHand"
SECOND_NAME = "SM_WallClock_SecondHand"

# Material slots: name -> (linear RGBA, metallic, roughness). Slot names become Unreal slot names.
MAT_DEFS = {
    "Wood": ((0.11, 0.06, 0.035, 1.0), 0.0, 0.5),
    "Dial": ((0.80, 0.80, 0.78, 1.0), 0.0, 0.6),
    "Ink": ((0.02, 0.02, 0.02, 1.0), 0.0, 0.5),
    "Glass": ((0.90, 0.95, 0.95, 1.0), 0.0, 0.05),
    "Hand": ((0.015, 0.015, 0.015, 1.0), 0.0, 0.4),
}
GLASS_OPACITY = 0.12
I_WOOD, I_DIAL, I_INK, I_GLASS = 0, 1, 2, 3

# Body (Unreal cm). Back (wall side) at Y_BACK, front crest of the wood ring at Y_FRONT.
R_OUT = 18.47            # outer radius of the wood ring
R_IN = 16.40             # inner radius of the ring (edge of the visible dial)
Y_BACK, Y_FRONT = -0.84, 1.02
RING_SEGMENTS = 80
DIAL_R = 16.55           # dial disc runs a little into the ring so no gap shows
DIAL_Y = (-0.80, -0.50)  # back, face
INK_Y = (-0.51, -0.47)   # numerals and ticks: 0.3 mm proud of the dial face (embedded 0.1 mm)
GLASS_R = 16.85          # glass disc, held in the rounded inner lip of the ring
GLASS_Y = (0.80, 0.86)

NUMERAL_R = 13.70        # numeral centers
NUMERAL_H = 2.40         # digit height
NUMERAL_BOLD = 0.030     # font outline offset (relative to a 1.0 font size) to make the digits bold;
                         # above ~0.04 the counters of 6/9 pinch and leave notches
NUMERAL_RES = 4          # curve resolution of the glyph outlines
NUMERAL_GAP = 0.10       # gap between the two digits of 10/11/12, relative to NUMERAL_H

TICK_R1 = 16.10          # outer end of all ticks
TICK_MIN = (0.42, 0.085)  # minute tick: length, width
TICK_HOUR = (0.75, 0.28)  # hour tick: length, width

# Hands (Unreal cm): Y layer (back, front), blade length to the tip, tail end. Hubs have radius HUB_R.
HUB_R = 0.60
TAIL_Z = -2.588
HOUR_Y = (-0.418, -0.267)
MINUTE_Y = (-0.267, -0.115)
SECOND_Y = (-0.115, 0.036)
HOUR_TIP = 7.885
MINUTE_TIP = 12.138
SECOND_TIP = 15.697
HUB_SEGMENTS = 20


def ue(x, y, z):
    """Unreal cm -> Blender m (Blender +Y = Unreal -Y)."""
    return Vector((x * 0.01, -y * 0.01, z * 0.01))


def set_mat(bm, mat_index):
    for f in bm.faces:
        f.material_index = mat_index
    return bm


def rotate_about_axis(bm, clock_deg):
    """Turn geometry clockwise (as seen from the front) about the clock axis (Unreal/Blender Y).
    Blender rotation about +Y by +a maps +Z toward +X, i.e. clockwise from the front (Blender -Y)."""
    a = math.radians(clock_deg)
    c, s = math.cos(a), math.sin(a)
    for v in bm.verts:
        x, z = v.co.x, v.co.z
        v.co.x = x * c + z * s
        v.co.z = -x * s + z * c
    return bm


# -----------------------------------------------------------------------------
# Geometry helpers
# -----------------------------------------------------------------------------


def revolve(profile, segments, mat_index, closed_loop):
    """Revolve a (r, y) profile (Unreal cm) around the clock axis (Unreal Y).
    Points with r == 0 become poles (triangle fans). closed_loop joins the last point back to the first."""
    bm = bmesh.new()
    rings = []
    for r, y in profile:
        if r < 1e-9:
            rings.append([bm.verts.new(ue(0.0, y, 0.0))])
        else:
            ring = []
            for k in range(segments):
                a = 2.0 * math.pi * k / segments
                ring.append(bm.verts.new(ue(r * math.sin(a), y, r * math.cos(a))))
            rings.append(ring)
    pairs = list(zip(rings[:-1], rings[1:]))
    if closed_loop:
        pairs.append((rings[-1], rings[0]))
    for a, b in pairs:
        if len(a) == 1 and len(b) == 1:
            continue
        for k in range(segments):
            k2 = (k + 1) % segments
            if len(a) == 1:
                bm.faces.new((a[0], b[k], b[k2]))
            elif len(b) == 1:
                bm.faces.new((a[k], a[k2], b[0]))
            else:
                bm.faces.new((a[k], a[k2], b[k2], b[k]))
    return set_mat(bm, mat_index)


def rounded_rect_profile(r0, r1, y0, y1, r_back_in, r_back_out, r_front_out, r_front_in, segs):
    """Closed (r, y) loop of a rectangle with four corner radii; segs = (back, front) segments per corner."""
    corners = [
        (r1 - r_back_out, y0 + r_back_out, r_back_out, 270.0, segs[0]),
        (r1 - r_front_out, y1 - r_front_out, r_front_out, 0.0, segs[1]),
        (r0 + r_front_in, y1 - r_front_in, r_front_in, 90.0, segs[1]),
        (r0 + r_back_in, y0 + r_back_in, r_back_in, 180.0, segs[0]),
    ]
    pts = []
    for cr, cy, rad, a0, n in corners:
        for i in range(n + 1):
            a = math.radians(a0 + 90.0 * i / n)
            pts.append((cr + rad * math.cos(a), cy + rad * math.sin(a)))
    return pts


def disc_y(radius, y0, y1, mat_index, segments):
    """Closed disc (cylinder) on the clock axis between Unreal y0 and y1 (cm)."""
    center = ue(0.0, (y0 + y1) * 0.5, 0.0)
    return rh.cylinder(radius * 0.01, (y1 - y0) * 0.01, tuple(center), axis="Y",
                       mat_index=mat_index, segments=segments)


def prism(outline, y0, y1, mat_index):
    """Extrude a convex (x, z) outline (Unreal cm) between Unreal y0 and y1."""
    bm = bmesh.new()
    back = [bm.verts.new(ue(x, y0, z)) for x, z in outline]
    front = [bm.verts.new(ue(x, y1, z)) for x, z in outline]
    n = len(outline)
    bm.faces.new(back)
    bm.faces.new(list(reversed(front)))
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((back[i], back[j], front[j], front[i]))
    return set_mat(bm, mat_index)


def disc_xz(radius, cx, cz, y0, y1, mat_index, segments=12):
    """Small closed disc lying in the dial plane, centered at Unreal (cx, cz)."""
    outline = [(cx + radius * math.sin(2.0 * math.pi * k / segments),
                cz + radius * math.cos(2.0 * math.pi * k / segments)) for k in range(segments)]
    return prism(outline, y0, y1, mat_index)


# -----------------------------------------------------------------------------
# Numerals (Blender's built-in font, converted to closed mesh shells)
# -----------------------------------------------------------------------------


def glyph_flat(ch):
    """Flat, triangulated fill of one character of Blender's built-in font (glyph units: X right, Y up,
    all characters on the same baseline). Blender's own extrusion of an offset (bold) outline self-intersects
    in the counters of 4/6/9, while the flat fill stays a clean patch, so numeral() extrudes it itself."""
    cu = bpy.data.curves.new("_Glyph", type="FONT")
    cu.body = ch
    cu.size = 1.0
    cu.resolution_u = NUMERAL_RES
    cu.offset = NUMERAL_BOLD
    cu.extrude = 0.0
    cu.fill_mode = "FRONT"
    ob = bpy.data.objects.new("_Glyph", cu)
    bpy.context.scene.collection.objects.link(ob)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    me = bpy.data.meshes.new_from_object(ob.evaluated_get(depsgraph))
    flat = bmesh.new()
    flat.from_mesh(me)
    bpy.data.objects.remove(ob, do_unlink=True)
    bpy.data.curves.remove(cu)
    bpy.data.meshes.remove(me)
    bmesh.ops.remove_doubles(flat, verts=flat.verts, dist=1e-6)
    bmesh.ops.triangulate(flat, faces=flat.faces, quad_method="BEAUTY", ngon_method="BEAUTY")
    if ch == "1":
        add_one_flag(flat)
    return flat


def add_one_flag(flat):
    """The built-in font's '1' is a bare stem, which makes 11 read like II. Add a separate flag stroke
    (its own closed island after extrusion) slanting down-left from the top of the stem, like the original."""
    xs = [v.co.x for v in flat.verts]
    ys = [v.co.y for v in flat.verts]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    stem_w, h = x1 - x0, y1 - y0
    a = math.radians(33.0)
    d = Vector((-math.cos(a), -math.sin(a), 0.0))   # along the flag, down-left
    p = Vector((math.sin(a), -math.cos(a), 0.0))    # across the flag, toward the stem
    length, thick = 0.30 * h, 0.80 * stem_w
    top = Vector((x0 + 0.45 * stem_w, y1, 0.0))
    quad = [top, top + d * length, top + d * length + p * thick, top + p * thick]
    flat.faces.new([flat.verts.new(c) for c in quad])


def glyph_bounds(flat):
    xs = [v.co.x for v in flat.verts]
    ys = [v.co.y for v in flat.verts]
    return min(xs), max(xs), min(ys), max(ys)


def extrude_flat(flat, to_ue, y0, y1, mat_index):
    """Closed slab from a flat patch: front cap at Unreal y1 (toward the reader), back cap at y0, side walls
    along the boundary edges. to_ue maps glyph (x, y) to Unreal (x, z)."""
    bm = bmesh.new()
    front, back = {}, {}
    for v in flat.verts:
        x, z = to_ue(v.co.x, v.co.y)
        front[v] = bm.verts.new(ue(x, y1, z))
        back[v] = bm.verts.new(ue(x, y0, z))
    for f in flat.faces:
        bm.faces.new([front[v] for v in f.verts])
        bm.faces.new([back[v] for v in reversed(f.verts)])
    for e in flat.edges:
        if len(e.link_faces) == 1:
            a, b = e.verts
            bm.faces.new((front[a], front[b], back[b], back[a]))
    return set_mat(bm, mat_index)


def numeral(text, cx, cz, height, y0, y1, mat_index):
    """Upright numeral readable from the front, centered at Unreal (cx, cz), between Unreal y0 (back) and y1.
    Digits are laid out here (tighter than the font's advance); height is the stem height of '1'."""
    ref = glyph_flat("1")
    _, _, ry0, ry1 = glyph_bounds(ref)
    ref.free()
    s = height / (ry1 - ry0)
    mid_y = (ry0 + ry1) * 0.5
    glyphs = [glyph_flat(ch) for ch in text]
    bounds = [glyph_bounds(g) for g in glyphs]
    gap = NUMERAL_GAP * height
    total = sum((b[1] - b[0]) * s for b in bounds) + gap * (len(glyphs) - 1)
    cursor = cx - total * 0.5
    bm = bmesh.new()
    for g, b in zip(glyphs, bounds):
        left = cursor

        def to_ue(gx, gy, left=left, gx0=b[0]):
            return left + (gx - gx0) * s, cz + (gy - mid_y) * s

        rh.merge_into(bm, extrude_flat(g, to_ue, y0, y1, mat_index))
        cursor += (b[1] - b[0]) * s + gap
        g.free()
    return bm


# -----------------------------------------------------------------------------
# Body
# -----------------------------------------------------------------------------


def build_body(mats):
    bm = bmesh.new()

    # Wood ring: rounded profile, nearly half-round toward the front, small radii at the wall side.
    profile = rounded_rect_profile(R_IN, R_OUT, Y_BACK, Y_FRONT,
                                   r_back_in=0.15, r_back_out=0.25, r_front_out=1.00, r_front_in=0.80,
                                   segs=(2, 6))
    rh.merge_into(bm, revolve(profile, RING_SEGMENTS, I_WOOD, closed_loop=True))

    # Dial disc (also closes the back of the clock).
    rh.merge_into(bm, disc_y(DIAL_R, DIAL_Y[0], DIAL_Y[1], I_DIAL, RING_SEGMENTS))

    # Minute track: 60 ticks, bold at the hours.
    for i in range(60):
        length, width = TICK_HOUR if i % 5 == 0 else TICK_MIN
        tick = prism([(-width * 0.5, TICK_R1 - length), (width * 0.5, TICK_R1 - length),
                      (width * 0.5, TICK_R1), (-width * 0.5, TICK_R1)], INK_Y[0], INK_Y[1], I_INK)
        rh.merge_into(bm, rotate_about_axis(tick, i * 6.0))

    # Numerals 1-12, upright.
    for h in range(1, 13):
        a = math.radians(h * 30.0)
        rh.merge_into(bm, numeral(str(h), NUMERAL_R * math.sin(a), NUMERAL_R * math.cos(a),
                                  NUMERAL_H, INK_Y[0], INK_Y[1], I_INK))

    # Flat glass cover.
    rh.merge_into(bm, disc_y(GLASS_R, GLASS_Y[0], GLASS_Y[1], I_GLASS, RING_SEGMENTS))

    return rh.finish_object(bm, BODY_NAME, [mats["Wood"], mats["Dial"], mats["Ink"], mats["Glass"]],
                            sharp_angle_deg=40.0)


# -----------------------------------------------------------------------------
# Hands (each at 12 o'clock, hub on the clock axis)
# -----------------------------------------------------------------------------


def inset(y_range, d=0.008):
    return (y_range[0] + d, y_range[1] - d)


def build_hour_hand(mats):
    bm = disc_y(HUB_R, HOUR_Y[0], HOUR_Y[1], 0, HUB_SEGMENTS)
    y0, y1 = inset(HOUR_Y)
    blade = [(-0.40, 0.0), (0.40, 0.0), (0.30, HOUR_TIP - 0.75), (0.0, HOUR_TIP), (-0.30, HOUR_TIP - 0.75)]
    rh.merge_into(bm, prism(blade, y0, y1, 0))
    tail = [(-0.26, TAIL_Z), (0.26, TAIL_Z), (0.36, 0.0), (-0.36, 0.0)]
    rh.merge_into(bm, prism(tail, y0, y1, 0))
    return rh.finish_object(bm, HOUR_NAME, [mats["Hand"]])


def build_minute_hand(mats):
    bm = disc_y(HUB_R, MINUTE_Y[0], MINUTE_Y[1], 0, HUB_SEGMENTS)
    y0, y1 = inset(MINUTE_Y)
    blade = [(-0.28, 0.0), (0.28, 0.0), (0.18, MINUTE_TIP - 0.65), (0.0, MINUTE_TIP), (-0.18, MINUTE_TIP - 0.65)]
    rh.merge_into(bm, prism(blade, y0, y1, 0))
    tail = [(-0.20, TAIL_Z), (0.20, TAIL_Z), (0.26, 0.0), (-0.26, 0.0)]
    rh.merge_into(bm, prism(tail, y0, y1, 0))
    return rh.finish_object(bm, MINUTE_NAME, [mats["Hand"]])


def build_second_hand(mats):
    # Domed center cap (rotationally symmetric) covering the hub stack of all three hands.
    cap = [(0.0, SECOND_Y[0]), (HUB_R, SECOND_Y[0]), (HUB_R, -0.075), (0.56, -0.035), (0.45, -0.005),
           (0.28, 0.022), (0.0, SECOND_Y[1])]
    bm = revolve(cap, HUB_SEGMENTS, 0, closed_loop=False)
    y0, y1 = SECOND_Y[0] + 0.006, -0.055
    needle = [(-0.075, 0.0), (0.075, 0.0), (0.035, SECOND_TIP), (-0.035, SECOND_TIP)]
    rh.merge_into(bm, prism(needle, y0, y1, 0))
    tail = [(-0.10, -2.0), (0.10, -2.0), (0.10, 0.0), (-0.10, 0.0)]
    rh.merge_into(bm, prism(tail, y0, y1, 0))
    # Round counterweight at the tail end.
    rh.merge_into(bm, disc_xz(0.30, 0.0, TAIL_Z + 0.30, y0, y1, 0, segments=12))
    return rh.finish_object(bm, SECOND_NAME, [mats["Hand"]])


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------


def stats(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [-v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    return {"min": [round(min(xs) * 100, 3), round(min(ys) * 100, 3), round(min(zs) * 100, 3)],
            "max": [round(max(xs) * 100, 3), round(max(ys) * 100, 3), round(max(zs) * 100, 3)],
            "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons),
            "slots": [m.name for m in obj.data.materials]}


def preview(objs, out_dir, pose=(10, 8, 30)):
    """Preview renders (after save/export, so the .blend stays clean):
    body at the thumbnail angle, and front views of the whole clock with posed copies of the hands."""
    body, hour, minute, second = objs
    for o in objs:
        o.hide_render = o is not body
    rh.preview_render(body, out_dir / f"{BODY_NAME}.png")
    for hand in (hour, minute, second):
        for o in objs:
            o.hide_render = o is not hand
        rh.preview_render(hand, out_dir / f"{hand.name}.png", azimuth_deg=90.0, elevation_deg=0.0)

    # Pose: Unreal pitch = -30*h, -6*m, -6*s  ->  clockwise angle seen from the front = Blender +Y rotation.
    hh, mm, ss = pose
    angles = {hour: 30.0 * (hh + mm / 60.0 + ss / 3600.0), minute: 6.0 * (mm + ss / 60.0), second: 6.0 * ss}
    copies = []
    for hand, ang in angles.items():
        c = hand.copy()
        bpy.context.scene.collection.objects.link(c)
        c.rotation_euler = (0.0, math.radians(ang), 0.0)
        c.hide_render = False
        copies.append(c)
    # Workbench draws the 12 % glass as a heavy haze, so the posed views use a body copy without the glass
    # faces (in Unreal the glass is a faint M_Prop_Glass instance).
    bare = body.copy()
    bare.data = body.data.copy()
    bpy.context.scene.collection.objects.link(bare)
    bm = bmesh.new()
    bm.from_mesh(bare.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index == I_GLASS], context="FACES")
    bm.to_mesh(bare.data)
    bm.free()
    for o in objs:
        o.hide_render = True
    bare.hide_render = False
    bpy.context.view_layer.update()
    tag = f"{hh:02d}{mm:02d}{ss:02d}"
    rh.preview_render([bare] + copies, out_dir / f"WallClock_Front_{tag}.png", resolution=768,
                      azimuth_deg=90.0, elevation_deg=0.0)
    rh.preview_render([bare] + copies, out_dir / f"WallClock_Thumb_{tag}.png")
    for c in copies + [bare]:
        bpy.data.objects.remove(c, do_unlink=True)


def main():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    preview_dir = Path(args[args.index("--preview") + 1]) if "--preview" in args else None

    EXPORT_DIR.mkdir(parents=True, exist_ok=True)
    rh.clear_scene()
    rh.set_scene(
        AssetPurpose="Self-made replacement for the MannDev wall clock (SM_Merged_Clock body and three hands).",
        Pivot="All four meshes: clock center at the origin. Front (dial) faces Unreal +Y = Blender -Y. "
              "Hands point to Unreal +Z (12 o'clock) and turn about Unreal Y (pitch).",
        ExportDir=str(EXPORT_DIR))
    mats = {name: rh.make_material(name, c, m, r) for name, (c, m, r) in MAT_DEFS.items()}
    glass = mats["Glass"]
    bsdf = next(n for n in glass.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Alpha"].default_value = GLASS_OPACITY
    c = MAT_DEFS["Glass"][0]
    glass.diffuse_color = (c[0], c[1], c[2], GLASS_OPACITY)
    if hasattr(glass, "surface_render_method"):
        glass.surface_render_method = "BLENDED"

    objs = [build_body(mats), build_hour_hand(mats), build_minute_hand(mats), build_second_hand(mats)]

    # Leftovers from the numeral conversion (fonts, curves) must not end up in the .blend.
    for coll in (bpy.data.curves, bpy.data.meshes, bpy.data.fonts):
        for block in list(coll):
            if block.users == 0:
                coll.remove(block)

    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT_BLEND))
    for obj in objs:
        rh.export_fbx(obj, EXPORT_DIR / f"{obj.name}.fbx")
        rh.report(obj)
    for backup in BASE_DIR.glob("*.blend1"):
        backup.unlink()
    print(f"Saved blend: {OUTPUT_BLEND}")

    if preview_dir is not None:
        preview_dir.mkdir(parents=True, exist_ok=True)
        (preview_dir / "stats.json").write_text(json.dumps({o.name: stats(o) for o in objs}, indent=1))
        preview(objs, preview_dir)


if __name__ == "__main__":
    main()
