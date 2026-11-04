# OfficeRoom textures

Self-made wall, carpet and ceiling textures for the `L_Office` room shell (`Art/Blender/OfficeRoom`).
They replace the engine default checker that the old BSP room used.

## Files

- `create_office_room_textures.py`: generates all six textures (Python 3 + numpy + Pillow).
- 1024 x 1024 PNGs, seamless in both directions:

| Texture | Content | One tile |
|---|---|---|
| `T_OfficeWall_BaseColor` / `_Normal` | warm off-white painted plaster (soft blotches, roller mottling, orange peel, a few specks) | 2 m |
| `T_OfficeCarpet_BaseColor` / `_Normal` | blue-grey 50 cm carpet tiles (2 x 2), quarter-turn pile direction, yarn flecks, thin seams | 1 m |
| `T_OfficeCeiling_BaseColor` / `_Normal` | 60 cm fissured acoustic tiles (2 x 2) in a white T-bar grid, rolled tile edges, rare water stains | 1.2 m |

Normal maps are DirectX style (Unreal). Carpet and ceiling seams sit at 0 / 0.5 along U and 0.25 / 0.75 along V,
so with the room mesh UVs (1 UV unit = 1 m, origin at the room center) the border tiles are symmetric.

Regenerate:

```
python create_office_room_textures.py
```

## Unreal

- Textures: `/Game/Objects/OfficeRoom/T_Office*` (normal maps import as `TC_Normalmap`, not sRGB).
- Material: `/Game/Objects/Materials/M_Prop_TexturedNormal` = `M_Prop_Textured` plus a `NormalTex` parameter
  (default `FlatNormal`) on the same tiled UVs.
- Instances in `/Game/Objects/OfficeRoom`:

| Instance | Textures | BaseColor (tint) | UVTiling | Roughness | GrimeAmount |
|---|---|---|---|---|---|
| `MI_OfficeRoom_Wall` | wall | 0.5 | 0.5 | 0.85 | 0.25 |
| `MI_OfficeRoom_Carpet` | carpet | 1.0 | 1.0 | 0.95 | 0.3 |
| `MI_OfficeRoom_Ceiling` | ceiling | 0.55 | 0.8333 | 0.9 | 0.15 |

The wall and ceiling textures are bright off-white; `L_Office` uses a fixed exposure (PostProcessVolume EV100 -0.5),
so their tint is lowered to keep them from blowing out to plain white.
