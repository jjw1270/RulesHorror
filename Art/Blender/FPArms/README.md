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

- The mesh is attached to the first-person camera. Its origin is the eye point. In the game the component is rotated 14° down around the eye (see "Unreal"), so the framing below is the unrotated rest framing.
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
  - a SkeletalMeshComponent attached to `First Person Camera`, relative rotation pitch -14° (location 0). This turns the arms down around the eye
    so they stay mostly out of view: while idle nothing shows on a 16:9 screen, while walking a fingertip swings in at the bottom now and then.
    The unrotated pose (hands held up in front) read as "zombie arms". Later interactions that need the hands can raise them again.
  - Only Owner See, first-person primitive type (rendered with the camera's 70° first-person FOV), no shadow, no collision
  - Animation Mode = Use Animation Blueprint with the native class `UFirstPersonArmsAnimInstance`
    (`Source/RulesHorror/GameFramework/Animation/FirstPersonArmsAnimInstance.*`, no Anim Blueprint asset). It reads
    `_FirstPersonIdleAnim` / `_FirstPersonWalkAnim` from the character (`A_FPArms_Idle` / `A_FPArms_Walk`) and blends
    them by ground speed: 100 % Walk at the movement's `MaxWalkSpeed` (250), walk play rate follows the speed (0.5 to 1.5),
    the blend weight eases in and out (`_BlendInterpSpeed` 8), and the walk cycle restarts from the first step after a stop.
- The mannequin is gone from `CharacterMesh0` and `First Person Mesh` (both have no mesh now). The camera used to hang on the `head` socket of `First Person Mesh`; `ARulesHorrorCharacter` now attaches it to the capsule at eye height, 64 cm above the capsule center (C++ default, the Blueprint keeps it).
- Import: `SkeletalMeshTools.import_file` with animations and without physics asset. The FBX uses the same axis conversion as the prop meshes (forward `-Z`, up `Y`), so Blender `+X` arrives as Unreal `+X`.
- The FBX is written in centimeters: `export_fbx` scales the rig data by 100 and sets the scene unit scale to 0.01 during the export, then restores the scene. Exported in meters, the FBX carries a x100 scale on the `Armature` node, and Unreal then imports either a mesh 100x too small (`FBX_SCALE_ALL`) or animation tracks that do not match the bind pose, which makes the arms disappear as soon as an animation plays (`FBX_SCALE_NONE`).

## 고퀄 팔로 바꾸는 법

팔은 `BP_RulesHorrorCharacter`의 `FP_Arms` 컴포넌트 하나에 모여 있다. 게임 코드는 팔의 뼈 이름이나 메시를 모르므로 이 컴포넌트와 캐릭터의 애니 값만 바꾸면 된다.

1. 새 팔을 가져온다(`/Game/Objects/` 아래 새 폴더). 저장소가 공개라 구매 에셋은 넣지 않는다. 직접 만들거나 CC0(예: MPFB 사람에서 팔만)로.
   스켈레탈 FBX는 cm 단위로 내보낸다(위 "Unreal"의 마지막 항목).
2. `FP_Arms`의 Skeletal Mesh를 새 메시로 바꾼다. Only Owner See, First Person 타입, 그림자·충돌 없음 설정은 그대로 둔다.
3. 애니는 둘 중 하나:
   - 간단히: Anim Class는 `FirstPersonArmsAnimInstance` 그대로 두고, 캐릭터의 `_FirstPersonIdleAnim` / `_FirstPersonWalkAnim`에 새 스켈레톤용 대기·걷기 시퀀스를 넣는다. 이동 속도에 따라 저절로 섞인다.
   - 애님 블루프린트: 새 팔에 맞는 애님 블루프린트를 만들어 `FP_Arms`의 Anim Class에 넣는다. 부모 클래스를 `FirstPersonArmsAnimInstance`로 하면
     그래프에서 `_WalkAlpha`(0 대기 ~ 1 걷기), `_WalkPlayRate`, `_IdleAnim`, `_WalkAnim`을 그대로 읽어 쓸 수 있다(그래프가 있으면 그래프가 포즈를 만든다).
4. `FP_Arms`의 상대 위치·회전으로 화면 속 자리를 맞춘다(지금은 pitch -14°). L_Street PIE에서 16:9로 대기·걷기 둘 다 확인한다.
   에디터 뷰포트는 16:9보다 넓어서 아래쪽이 덜 보이므로, PIE 중 `First Person Camera`의 Constrain Aspect Ratio를 켜고 Aspect Ratio 1.7778로 본다.
5. 옛 팔 에셋(`/Game/Objects/FPArms/`)은 아무것도 참조하지 않게 된 뒤에 지운다.
