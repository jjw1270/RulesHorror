# CeilingLamp

Self-made fluorescent ceiling fixture for RulesHorror.
Replaces the purchased `BrutalistLevelKit` `SM_Ceiling_Lamp`, which cannot be pushed to the public repository.

## Files

- `create_ceiling_lamp_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `CeilingLamp.blend`: Blender source. The scene contains only `SM_CeilingLamp`.
- `Exports/SM_CeilingLamp.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_ceiling_lamp_blend.py
```

## Geometry

- Size: `1.748 x 0.341 x 0.232 m`, same bounds as the original (Unreal cm: min `(-88.9, -24.5, -21.7)`, max `(85.9, 9.6, 1.5)`).
- Pivot: ceiling mount point at the top (ceiling tabs reach `z = +1.5 cm`), the fixture hangs below it. Long axis `X`.
  The opal diffuser is on the Unreal `+Y` side (Blender `-Y`); the mounting rail runs behind it on Unreal `-Y`.
  The pivot matches the original exactly because `/Game/Objects/Ceiling_Light_BP` places its light relative to it.
- Parts: rounded opal diffuser, metal back housing with a row of vent slots on top, round mounting rail,
  end caps, end brackets with rail clamps and ceiling tabs.
- About 2,800 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Tube_Emissive` | emissive instance: opal white (`0.85, 0.86, 0.85`, roughness 0.35), emissive warm-cool white (`1.0, 0.97, 0.92`) |
| `Metal_Housing` | `M_Prop_Base` instance, blue-grey painted metal (`0.45, 0.50, 0.55`, metallic 0.5, roughness 0.45) |
| `Metal_Rail` | `M_Prop_Base` instance, dark grey metal (`0.20, 0.21, 0.22`, metallic 0.7, roughness 0.4) |

`M_Prop_Base` parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`.

## Unreal

- Asset: `/Game/Objects/CeilingLamp/SM_CeilingLamp` (Nanite off, simple box collision is enough).
- Replaces `/Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/Lights/SM_Ceiling_Lamp` (used by `/Game/Objects/Ceiling_Light_BP`).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
