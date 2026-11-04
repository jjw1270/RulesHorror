# FPArms

Self-made first-person arms for RulesHorror: an office worker's forearms in a pale blue-white dress shirt with buttoned cuffs, and pale bare hands, rigged and animated.
They replace Epic's mannequin arms on the test player character. The arms are a skeletal mesh attached to the first-person camera.

## Files

- `create_fp_arms_blend.py`: generates the `.blend` and the FBX (mesh, skeleton, both actions). Run it again after changing it.
- `verify_fp_arms_fbx.py`: re-imports the exported FBX into an empty scene and prints bone count, bounds in Unreal cm, triangles, skin influences, winding check, and each animation take with its frame range and loop seam error.
- `FPArms.blend`: Blender source. The scene contains only `Armature` and `SKM_FPArms`, plus the actions `Idle` and `Walk` (fake user).
- `Exports/SKM_FPArms.fbx`: FBX for Unreal import (armature + mesh + both takes).

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_fp_arms_blend.py
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python verify_fp_arms_fbx.py
```

Optional arguments after `--` for the generator:
- `--previews`: renders first-person and outside previews to `Saved/ClaudeTmp/previews/FPArms/`.
- `--agent-json <path>`: writes the import spec JSON.

## Camera-space contract

- The mesh is attached to the first-person camera with an identity relative transform. Its origin is the eye point.
- Unreal axes: `+X` = view forward, `+Y` = right, `+Z` = up. Blender `+X` = UE `+X`, Blender `+Y` = UE `-Y`, Blender `Z` = UE `Z`, 1 m = 100 cm.
- The player's right arm (UE `+Y`, Blender `-Y`) uses the `_r` bones.
- Framing was made for a 70° horizontal FOV at 16:9 (about 43° vertical):
  - The hands sit in the lower-left and lower-right of the view, inside the bottom third and roughly the outer third of the screen.
  - The cuffs show at the bottom corners.
  - The elbows and upper arms are off screen.
- Rest-pose landmarks in UE cm:
  - wrist joints `(33.0, ±18.5, -10.5)`
  - middle-finger tips about `(49, ±11, -10)`
  - mesh bounds `min (-1.2, -29.2, -32.1)`, `max (49.6, 29.2, -6.1)`
- The shoulder joints are at `(-15, ±19, -19.5)`, a little further back than anatomical. This is a cheat so the forearms come in flatter and the cuffs stay in view; nothing above the elbow is ever on screen.

## Geometry

- 8,864 triangles for both arms, 4,534 vertices. Smooth shaded, box-projected UVs (no textures).
- Built as a low-poly cage of one arm in a straight canonical pose, given one Catmull-Clark subdivision level, then skinned into the final pose and mirrored for the other arm.
- Skin, per arm:
  - forearm stub hidden inside the sleeve
  - palm with thenar and hypothenar pads
  - four fingers with knuckle bumps
  - opposable thumb
  - thin nail plates on all five distal segments (Skin material)
  - hand about 18.5 cm long, forearm 26 cm
- Sleeve, per arm, a closed shell:
  - runs from mid upper arm (capped) to a 6 cm turned cuff, with a rolled rim and a short inner lining
  - a small dark cuff button on the pinky side
  - gentle fabric folds above the cuff and at the elbow
- Rest pose = the "cautious" idle pose:
  - hands raised slightly in front, palms facing inward and down, fingers relaxed and slightly curled
  - pinky curled most, index least; the thumb rests below the index finger

## Skeleton

The armature object is named `Armature`, so Unreal treats `root` as the root bone. `root` sits at the eye point with identity orientation. 37 bones, no leaf bones:

```
root
├─ upperarm_r ─ lowerarm_r ─ hand_r ─┬─ thumb_01_r ─ thumb_02_r ─ thumb_03_r
│                                    ├─ index_01_r ─ index_02_r ─ index_03_r
│                                    ├─ middle_01_r ─ middle_02_r ─ middle_03_r
│                                    ├─ ring_01_r ─ ring_02_r ─ ring_03_r
│                                    └─ pinky_01_r ─ pinky_02_r ─ pinky_03_r
└─ upperarm_l ─ ... (same with _l)
```

- Bones point along local `+Y` (FBX exported with primary `Y`, secondary `X`). For the hand and finger bones, local `Z` points to the back of the hand or nail.
- Finger flexion is a negative rotation around local `X` on both sides. The left side is the mirror image (local `X` axes point in opposite directions).
- `_01/_02/_03` are the proximal, middle and distal phalanges. For the thumb they are the metacarpal, proximal and distal.
- Skin weights are smooth across every joint, with at most 4 influences per vertex. Wrist, knuckle and elbow transitions blend over several edge loops.

## Actions (30 fps, seamless loops: first frame equals last frame)

| Action | FBX take | Frames | Length | Motion |
|---|---|---|---|---|
| `Idle` | `Armature\|Idle` | 0-120 (121 keys) | 4 s | slow breathing drift of the arms, about ±0.7 cm vertical with left and right out of phase; ±1-2° hand tilt; out-of-phase finger relax |
| `Walk` | `Armature\|Walk` | 0-30 (31 keys) | 1 s | arms alternate: about ±1.7 cm up/down and ±0.5 cm forward/back; hand flex ±2°; lagging finger sway (for a future Anim Blueprint) |

Only the `upperarm` bones have location keys (whole-arm drift); every other bone has rotation keys only.

## Material slots

| Slot | Unreal material | Base color (linear) | Roughness | Grime |
|---|---|---|---|---|
| `Sleeve` | `/Game/Objects/FPArms/MI_FPArms_Sleeve` (`opaque`) | `0.55, 0.60, 0.66` | 0.80 | 0.1 |
| `Skin` | `/Game/Objects/FPArms/MI_FPArms_Skin` (`opaque`) | `0.55, 0.38, 0.30` | 0.55 | 0.0 |
| `Button` | `/Game/Objects/FPArms/MI_FPArms_Button` (`opaque`) | `0.10, 0.10, 0.105` | 0.35 | 0.1 |

- All three are instances of `/Game/Objects/Materials/M_Prop_Base`, with parameters `BaseColor`, `Metallic` (0), `Roughness` and `GrimeAmount`.
- `M_Prop_Base` has `Used with Skeletal Mesh` enabled for these instances.

## Unreal

- Assets in `/Game/Objects/FPArms/`: `SKM_FPArms`, `SKM_FPArms_Skeleton`, `A_FPArms_Idle`, `A_FPArms_Walk` (the imported takes renamed), and the three `MI_FPArms_*` instances. No physics asset.
- Used by `/Game/GameFramework/Pawn/BP_RulesHorrorCharacter` (test pawn of `L_InGameTest`), component `FP_Arms`:
  - a SkeletalMeshComponent attached to `First Person Camera`, relative transform identity
  - Only Owner See, first-person primitive type (rendered with the camera's 70° first-person FOV), no shadow, no collision
  - Animation Mode = Use Animation Asset, `A_FPArms_Idle`, looping (`A_FPArms_Walk` is not used yet)
- The mannequin is gone from `CharacterMesh0` and `First Person Mesh` (both have no mesh now). The camera was attached to the `head` socket of `First Person Mesh`; it now sits at the eye point, 64 cm above the capsule center (relative location `(0, 20, 160)` in the mesh frame, rotation `(0, 90, 0)`). The C++ attachment still names the `head` socket, so spawning logs a few harmless `GetSocketInfoByName(head): No SkeletalMesh` warnings.
- Import: `SkeletalMeshTools.import_file` with animations and without physics asset. The FBX uses the same axis conversion as the prop meshes (forward `-Z`, up `Y`), so Blender `+X` arrives as Unreal `+X`.
- The FBX is written in centimeters: `export_fbx` scales the rig data by 100 and sets the scene unit scale to 0.01 during the export, then restores the scene. Exported in meters, the FBX carries a x100 scale on the `Armature` node, and Unreal then imports either a mesh 100x too small (`FBX_SCALE_ALL`) or animation tracks that do not match the bind pose, which makes the arms disappear as soon as an animation plays (`FBX_SCALE_NONE`).
