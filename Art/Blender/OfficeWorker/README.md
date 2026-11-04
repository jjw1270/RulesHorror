# OfficeWorker

Seated office worker for the `L_Office` player, seen only in reflections (the dark CRT glass) and shadows.
The office player (`OfficePawn_BP`, C++ `AOfficePawn`) is a camera without a body; these meshes give it one.

The character is generated with [MPFB](https://github.com/makehumancommunity/mpfb2) (MakeHuman for Blender):
a young Asian man in a dark suit, white shirt and tie, black dress shoes and short dark hair, posed sitting on the
`L_Office` office chair with his hands on his thighs.

## Sources and license

- Body, skin, eyes, eyebrows, eyelashes, hair, suit and shoes come from the MakeHuman base mesh and the
  `makehuman_system_assets` asset pack. Both are **CC0** (public domain); models exported from MPFB are CC0 as well.
- MPFB itself is GPL-3.0. It is only a tool used to generate the files here; none of its code is in this repository.

## Files

- `create_office_worker_blend.py`: builds the character with MPFB, poses it, splits it into head and body, writes the
  `.blend`, both FBX files and the textures. Run it again after changing it.
- `OfficeWorker.blend`: Blender source (`SM_OfficeWorker_Head`, `SM_OfficeWorker_Body`, materials use `Textures/`).
- `Exports/SM_OfficeWorker_Head.fbx`, `Exports/SM_OfficeWorker_Body.fbx`: FBX for Unreal import.
- `Textures/T_OfficeWorker_*.png`: the MakeHuman textures, box-filtered down (skin, suit, hair 1024; eye, shoes 512;
  eyebrows, eyelashes 256). Hair, eyebrows and eyelashes keep their alpha.

## Setup (once)

MPFB is installed into a separate Blender user folder so the normal Blender setup is not touched:

1. Download MPFB 2.0.18 (`https://github.com/makehumancommunity/mpfb2/archive/refs/tags/v2.0.18.tar.gz`), zip the
   contents of `src/mpfb` (the folder with `blender_manifest.toml`) as `mpfb-2.0.18.zip`.
2. Install it:
   ```
   set BLENDER_USER_RESOURCES=C:\Users\<you>\blender_mpfb\user
   "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --command extension install-file -r user_default -e mpfb-2.0.18.zip
   ```
3. Download `makehuman_system_assets_cc0.zip`
   (`https://files.makehumancommunity.org/asset_packs/makehuman_system_assets/makehuman_system_assets_cc0.zip`)
   and unzip it into `%BLENDER_USER_RESOURCES%\extensions\.user\user_default\mpfb\data`
   (folders `clothes`, `eyes`, `hair`, `skins`, ... directly inside `data`).

## Regenerate

```
set BLENDER_USER_RESOURCES=C:\Users\<you>\blender_mpfb\user
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python create_office_worker_blend.py
```

Append `-- --preview <dir>` to also render previews (`monitor`, `face`, `front`, `side` with the chair, desk and
monitor, `back`) and `--no-save` to skip writing the `.blend` and the FBX files.

## How it is built

- MPFB human: male, Asian, age 0.5 (about 25), weight 0.55, height 0.7; skin `young_asian_male`; eyes `high-poly`
  (brown); `eyebrow001`, `eyelashes01`, hair `short02`, clothes `male_elegantsuit01` and `shoes04`; no subdivision;
  `game_engine` rig.
- Pose: the bones are aimed in world space (`POSE` table): thighs forward, shins down, arms hanging slightly forward,
  forearms and hands on the thighs; small spine and neck tilts (`SPINE`). The thigh slope is solved so that the soles
  rest on the floor 121 cm below the eyes (the `MP_Main` eye height); the seat lands on the chair cushion.
- The posed meshes are baked (armature, the masks that hide skin under the clothes). The skin is split at the base of the
  neck by bone weights (`head` + `neck_01` = head part); the cut sits inside the shirt collar.
- The eye texture leaves the cornea shell transparent; those faces are removed so the eyes can use an opaque material.
- Neck filler (`NECK_FILLER`): the skin of the lower neck and upper chest inside the collar is kept on the body, shrunk
  3 mm inwards. At rest the head's own neck covers it; when the head turns it fills the gap between collar and neck.
- `HEAD_PIVOT` `(-10, 0, -13)` cm: the middle of the neck at collar height (the script prints the `neck_01` and `head`
  bone positions to choose it). It must match `AOfficePawn::_HeadPivot`.
- Both meshes have their origin at the midpoint between the eyeballs (the pawn camera), Unreal `+X` forward, `+Z` up.
  - `SM_OfficeWorker_Head`: about 13,500 triangles, bounds `(-20.8, -9.5, -19.4) .. (3.7, 10.0, 13.4)` cm:
    head and neck skin, eyes, eyebrows, eyelashes, hair.
  - `SM_OfficeWorker_Body`: about 25,800 triangles, bounds `(-26.4, -29.0, -120.6) .. (64.8, 29.2, -12.0)` cm:
    hands, suit, shoes, neck filler (the other skin under the clothes is removed).

## Material slots

| Slot | Unreal material | Parent | Tint | Roughness |
|---|---|---|---|---|
| `Skin` | `MI_OfficeWorker_Skin` | `M_Prop_Textured` | `1.0, 0.94, 0.9` | 0.55 |
| `Eye` | `MI_OfficeWorker_Eye` | `M_Prop_Textured` | white | 0.15 |
| `Suit` | `MI_OfficeWorker_Suit` | `M_Prop_Textured` | white | 0.7 |
| `Shoes` | `MI_OfficeWorker_Shoes` | `M_Prop_Textured` | white | 0.35 |
| `Hair` | `MI_OfficeWorker_Hair` | `M_Prop_TexturedMasked` | `0.32, 0.3, 0.29` (darkens the brown hair) | 0.5 |
| `Eyebrows` | `MI_OfficeWorker_Eyebrows` | `M_Prop_TexturedMasked` | `0.45, 0.42, 0.4` | 0.6 |
| `Eyelashes` | `MI_OfficeWorker_Eyelashes` | `M_Prop_TexturedMasked` | `0.4` | 0.6 |

All instances use `T_OfficeWorker_<Slot>` as `BaseColorTex`, `UVTiling` 1 and `GrimeAmount` 0.
`/Game/Objects/Materials/M_Prop_TexturedMasked` is a copy of `M_Prop_Textured` with blend mode `Masked`, two sided,
and the alpha of `BaseColorTex` as the opacity mask.

## Unreal

- Assets: `/Game/Objects/OfficeWorker/SM_OfficeWorker_Head`, `SM_OfficeWorker_Body` (no collision, Nanite off),
  textures `T_OfficeWorker_*`, instances `MI_OfficeWorker_*`.
- `AOfficePawn` components (C++): `ReflectionHead` is attached to the camera and turns with the view, but about the
  neck (`_HeadPivot`, body space from the eyes) instead of the eyes, so the neck stays inside the collar for the whole
  look range (yaw -5 .. 50, pitch -5 .. 15 around the move point's pitch); `ReflectionBody` is attached to the pawn root with absolute rotation, stays upright and follows only 35 % of the
  look yaw (`_BodyYawFollowRatio`). Both use `FirstPersonPrimitiveType = WorldSpaceRepresentation`: hidden from the
  player's own view, still visible in hardware ray traced (Lumen) reflections and shadows, no collision.
  The meshes are assigned in `OfficePawn_BP` and on the pawn placed in `L_Office`.
- When the pawn leans in to `MP_Computer`, the lower body passes through the desk; only the head and shoulders are
  ever reflected in the screen, and the desk hides the rest.
