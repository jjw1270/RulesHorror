# LightSwitch

Self-made wall switch combination (two stacked wall plates) for RulesHorror.
Replaces the purchased `BrutalistLevelKit` `SM_Light_Switch3`, which cannot be pushed to the public repository.

## Files

- `create_light_switch_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `LightSwitch.blend`: Blender source. The scene contains only `SM_LightSwitch`.
- `Exports/SM_LightSwitch.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_light_switch_blend.py
```

## Geometry

- Size: `0.127 x 0.043 x 0.261 m`, same bounds as the original (Unreal cm: min `(-6.3, -0.4, -6.3)`, max `(6.4, 3.9, 19.8)`).
- Pivot: center of the lower plate on the wall plane. The plates stick out toward Unreal `+Y` (Blender `-Y`), `Z` up; the back sinks 4 mm into the wall like the original.
- Parts: two rounded frames with soft front bevels; upper plate with two tilted rocker switches; lower plate with a square insert holding a round socket recess (two pin holes, earthing clips).
- About 1,700 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Plastic_Plate` | `M_Prop_Base` instance, off-white plastic (`0.60, 0.60, 0.56`, metallic 0, roughness 0.38) |
| `Plastic_Insert` | `M_Prop_Base` instance, slightly whiter plastic (`0.68, 0.68, 0.65`, metallic 0, roughness 0.30) |
| `Socket_Contact` | `M_Prop_Base` instance, near black (`0.012`, metallic 0, roughness 0.6) for pin holes and clips |

All are instances of `/Game/Objects/Materials/M_Prop_Base` (parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`).

## Unreal

- Asset: `/Game/Objects/LightSwitch/SM_LightSwitch` (Nanite off, simple box collision is enough).
- Replaces `/Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/SM_Light_Switch3` (used in `L_Office`).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
