# FileCabinet

Self-made vertical metal filing cabinet for RulesHorror: teal-green painted steel, 3 drawers, each with a card label holder and a hanging bail pull.
Replaces the purchased `BrutalistLevelKit` `SM_File_Cabinet1`, which cannot be pushed to the public repository.

## Files

- `create_file_cabinet_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `FileCabinet.blend`: Blender source. The scene contains only `SM_FileCabinet`.
- `Exports/SM_FileCabinet.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_file_cabinet_blend.py
```

Append `-- --preview <dir>` to also render a thumbnail-angle preview (not saved into the `.blend`).

## Geometry

- Size: `0.54 x 0.78 x 1.00 m` (Unreal bounds x -27.1 .. 27.1, y 0.6 .. 78.8, z 0 .. 99.9 cm), same as the original within 0.3 cm.
- Pivot: same as the original: floor level, centered in X, back face at Unreal `y = 0.6`. Drawers face Unreal `+Y` (Blender `-Y`).
- Parts: carcass with a slightly overhanging top cap, dark recessed kick, 3 drawer fronts with rolled edges,
  label holders (metal frame + paper card), drop-bail pulls hanging forward at 40 degrees.
- About 2,100 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Body_Paint` | new `M_Prop_Base` instance: BaseColor `(0.055, 0.15, 0.13)`, Metallic 0.3, Roughness 0.5, GrimeAmount 0.45 |
| `Metal_Dark` | new `M_Prop_Base` instance: BaseColor `(0.05, 0.055, 0.06)`, Metallic 1, Roughness 0.4, GrimeAmount 0.3 (pulls, label frames, kick) |
| `Label_Paper` | new `M_Prop_Base` instance: BaseColor `(0.40, 0.38, 0.32)`, Metallic 0, Roughness 0.85, GrimeAmount 0.5 (blank label cards) |

Base colors are linear. All are instances of `/Game/Objects/Materials/M_Prop_Base`.

## Unreal

- Asset: `/Game/Objects/FileCabinet/SM_FileCabinet`.
- The original is used in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
