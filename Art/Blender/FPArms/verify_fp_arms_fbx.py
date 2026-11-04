"""Re-import Exports/SKM_FPArms.fbx into an empty Blender scene and print what Unreal will receive:
bone count/names, mesh bounds in Unreal cm (camera space), triangles, material slots, skin influences,
and every animation take with its frame range, loop seam error and hand motion range.

  "C:\\Program Files\\Blender Foundation\\Blender 5.1\\blender.exe" --background --factory-startup \
      --python verify_fp_arms_fbx.py [-- --json <report.json>]
"""

import json
import sys
from pathlib import Path

import bpy
from mathutils import Vector

BASE_DIR = Path(__file__).resolve().parent
FBX_PATH = BASE_DIR / "Exports" / "SKM_FPArms.fbx"

EXPECTED_BONES = {"root"} | {
    f"{name}_{side}"
    for side in ("l", "r")
    for name in ["upperarm", "lowerarm", "hand"]
    + [f"{finger}_0{i}" for finger in ("thumb", "index", "middle", "ring", "pinky") for i in (1, 2, 3)]
}


def ue(v):
    """Blender world (meters) -> Unreal camera space (cm)."""
    return (v.x * 100.0, -v.y * 100.0, v.z * 100.0)


def set_action(obj, act):
    obj.animation_data.action = act
    if obj.animation_data.action_slot is None and len(act.slots):
        obj.animation_data.action_slot = act.slots[0]


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    json_path = Path(argv[argv.index("--json") + 1]) if "--json" in argv else None

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=str(FBX_PATH))
    scene = bpy.context.scene
    arms = [o for o in scene.objects if o.type == "ARMATURE"]
    meshes = [o for o in scene.objects if o.type == "MESH"]
    report = {"fbx": FBX_PATH.as_posix(), "objects": {}, "errors": []}
    for o in scene.objects:
        report["objects"][o.name] = {
            "type": o.type, "parent": o.parent.name if o.parent else None,
            "location": [round(c, 5) for c in o.location],
            "rotation_deg": [round(c * 57.29578, 3) for c in o.rotation_euler],
            "scale": [round(c, 5) for c in o.scale]}
    if len(arms) != 1 or len(meshes) != 1:
        report["errors"].append(f"expected 1 armature + 1 mesh, got {len(arms)} + {len(meshes)}")
    arm, mesh = arms[0], meshes[0]

    names = {b.name for b in arm.data.bones}
    report["bones"] = len(names)
    report["root_bones"] = [b.name for b in arm.data.bones if b.parent is None]
    if names != EXPECTED_BONES:
        report["errors"].append(f"bone mismatch: missing {sorted(EXPECTED_BONES - names)} "
                                f"extra {sorted(names - EXPECTED_BONES)}")

    # Rest-pose mesh in world space -> Unreal cm.
    for o in (arm,):
        o.animation_data.action = None if o.animation_data else None
    for pb in arm.pose.bones:
        pb.matrix_basis.identity()
    bpy.context.view_layer.update()
    pts = [ue(mesh.matrix_world @ v.co) for v in mesh.data.vertices]
    report["ue_bounds_cm"] = {"min": [round(min(p[i] for p in pts), 2) for i in range(3)],
                              "max": [round(max(p[i] for p in pts), 2) for i in range(3)]}
    report["tris"] = sum(len(p.vertices) - 2 for p in mesh.data.polygons)
    report["slots"] = [m.name for m in mesh.data.materials]
    influences = [sum(1 for g in v.groups if g.weight > 1e-4) for v in mesh.data.vertices]
    report["max_influences"] = max(influences)
    report["unweighted_verts"] = sum(1 for n in influences if n == 0)
    groups = {g.name for g in mesh.vertex_groups}
    report["vertex_groups_not_bones"] = sorted(groups - names)
    # Winding check: signed volume of the closed parts per side must be positive (normals point outward).
    vol = {}
    for poly in mesh.data.polygons:
        mat = mesh.data.materials[poly.material_index].name
        co = [mesh.matrix_world @ mesh.data.vertices[i].co for i in poly.vertices]  # meters, whatever the FBX unit
        side = "l" if sum(c.y for c in co) > 0 else "r"
        for k in range(1, len(co) - 1):
            vol[(side, mat)] = vol.get((side, mat), 0.0) + co[0].dot(co[k].cross(co[k + 1])) / 6.0
    report["signed_volume_cm3"] = {f"{s}/{m}": round(v * 1e6, 1) for (s, m), v in sorted(vol.items())}
    if any(v <= 0 for v in vol.values()):
        report["errors"].append("negative signed volume: some faces point inward")
    edge_faces = {}
    for poly in mesh.data.polygons:
        for ek in poly.edge_keys:
            edge_faces[ek] = edge_faces.get(ek, 0) + 1
    report["open_edges"] = sum(1 for n in edge_faces.values() if n == 1)  # nail plates are open by design
    report["nonmanifold_edges"] = sum(1 for n in edge_faces.values() if n > 2)
    hands = {s: ue(arm.matrix_world @ arm.data.bones[f"hand_{s}"].head_local) for s in ("l", "r")}
    report["rest_wrist_ue_cm"] = {s: [round(c, 2) for c in p] for s, p in hands.items()}

    # Animation takes.
    report["actions"] = {}
    if arm.animation_data is None:
        arm.animation_data_create()
    for act in bpy.data.actions:
        set_action(arm, act)
        f0, f1 = (int(round(f)) for f in act.frame_range)
        samples = {}
        for f in range(f0, f1 + 1):
            scene.frame_set(f)
            samples[f] = {pb.name: (arm.matrix_world @ pb.matrix).copy() for pb in arm.pose.bones}
        seam = max((samples[f0][n].translation - samples[f1][n].translation).length for n in samples[f0])
        seam_rot = max(samples[f0][n].to_quaternion().rotation_difference(samples[f1][n].to_quaternion()).angle
                       for n in samples[f0])
        motion = {}
        for s in ("l", "r"):
            heads = [Vector(ue(samples[f][f"middle_01_{s}"].translation)) for f in samples]
            motion[s] = [round(max(h[i] for h in heads) - min(h[i] for h in heads), 2) for i in range(3)]
        report["actions"][act.name] = {
            "frame_range": [f0, f1], "frames": f1 - f0 + 1, "seconds_at_30fps": round((f1 - f0) / 30.0, 3),
            "loop_seam_cm": round(seam * 100.0, 4), "loop_seam_deg": round(seam_rot * 57.29578, 4),
            "knuckle_motion_range_ue_cm": motion}

    text = json.dumps(report, indent=2)
    print("FPARMS_VERIFY_BEGIN")
    print(text)
    print("FPARMS_VERIFY_END")
    if json_path is not None:
        json_path.parent.mkdir(parents=True, exist_ok=True)
        json_path.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
