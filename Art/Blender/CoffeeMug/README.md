# CoffeeMug

Self-made red coffee mug with leftover coffee for RulesHorror.
Replaces the purchased `Deko_MatrixDemo` `SM_Food_Leftover_EO1_N1`, which cannot be pushed to the public repository.

## Files

- `create_coffee_mug_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `CoffeeMug.blend`: Blender source. The scene contains only `SM_CoffeeMug`.
- `Exports/SM_CoffeeMug.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_coffee_mug_blend.py
```

## Geometry

- Size: `0.09 x 0.12 x 0.097 m`, same bounds as the original (Unreal X -4.5..4.5, Y -6.0..6.0, Z 0..9.7 cm).
- Pivot: center of the bounding box (body + handle) at the bottom, `Z` up. The body axis sits at Unreal `(0, -1.5)`.
- Facing: like the original, the handle points to Unreal `+Y` turned 28 degrees toward `+X` (Blender `-Y` turned toward `+X`).
  The original's bounds and its saved thumbnail camera (orbit yaw -54.5, pitch -18.25) both show this.
- Parts: lathed body (32 segments) with recessed foot, hollow inside, rolled rim bead, coffee surface 1.9 cm above the floor
  with a thin stain line above it and a dried ring higher on the inner wall; swept flat-oval handle.
- About 2,200 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Ceramic_Glaze` | new instance of `M_Prop_Base`: red glaze, BaseColor `(0.30, 0.012, 0.018)`, Roughness `0.22` |
| `Ceramic_Rim` | new instance of `M_Prop_Base`: off-white rim, BaseColor `(0.80, 0.75, 0.68)`, Roughness `0.35` |
| `Coffee_Leftover` | new instance of `M_Prop_Base`: dark coffee, BaseColor `(0.018, 0.007, 0.003)`, Roughness `0.12`, low grime |

## Unreal

- Asset: `/Game/Objects/CoffeeMug/SM_CoffeeMug` (Nanite off, 1 convex hull collision).
- Replaces `SM_Food_Leftover_EO1_N1` in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
