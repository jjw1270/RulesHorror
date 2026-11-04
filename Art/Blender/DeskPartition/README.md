# DeskPartition

Self-made office cubicle divider for RulesHorror: a framed teal panel with a strip of dirty glass on top.
Replaces the purchased `BrutalistLevelKit` `SM_Desk_Partition_4X`, which cannot be pushed to the public repository.

## Files

- `create_desk_partition_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `DeskPartition.blend`: Blender source. The scene contains only `SM_DeskPartition`.
- `Exports/SM_DeskPartition.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_desk_partition_blend.py
```

Append `-- --preview <dir>` to also render a thumbnail-angle preview (not saved into the `.blend`).

## Geometry

- Size: `2.00 x 0.06 x 1.50 m` (Unreal bounds x -100.5 .. 99.5, y -3 .. 3, z 0 .. 150.2 cm), identical to the original.
- Pivot: same as the original: floor level, center of the thickness; the width runs from x -100.5 to 99.5 cm (not centered).
  Both faces are identical.
- Parts: 6 cm frame (stiles and rails, 0 .. 87 cm), recessed infill panel with a decorative bead rectangle on both faces,
  glazing beads, 1.6 cm dirty glass pane from 85 to 150.2 cm.
- About 930 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Panel_Plastic` | new `M_Prop_Base` instance: BaseColor `(0.10, 0.29, 0.235)`, Metallic 0, Roughness 0.6, GrimeAmount 0.35 |
| `Glass` | new `M_Prop_Glass` instance: BaseColor `(0.42, 0.47, 0.45)`, Opacity 0.35, Roughness 0.35 (dirty glass) |

Base colors are linear.

## Unreal

- Asset: `/Game/Objects/DeskPartition/SM_DeskPartition`.
- The original is used in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
