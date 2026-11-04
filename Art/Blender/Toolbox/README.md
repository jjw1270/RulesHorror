# Toolbox

Self-made open tool tote (blue painted boards, center handle bar) full of tools for RulesHorror.
Replaces the purchased `Deko_MatrixDemo` `SM_Toolbox_Prestaged_A01_N1`, which cannot be pushed to the public repository.

## Files

- `create_toolbox_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `Toolbox.blend`: Blender source. The scene contains only `SM_Toolbox`.
- `Exports/SM_Toolbox.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_toolbox_blend.py
```

## Geometry

- Size: `0.54 x 0.35 x 0.23 m`, same bounds as the original (Unreal X -27.6..26.6, Y -20.2..15.0, Z 0..22.9 cm).
- Pivot: bottom center of the box (same as the original), `Z` up. Long side and handle bar along `X`;
  the side facing Unreal `+Y` is the front (front compartment), `-Y` the back.
- Box: 50 x 30 cm tray of beveled boards, 11 cm walls, 14.5 cm center divider, two end posts up to 22.9 cm with a
  steel handle bar between them.
- Tools: screwdriver leaning out over the `-X` end, screwdriver resting on the back rim with its handle outside
  (gives the `-Y` bound), claw hammer, adjustable wrench and pliers standing in the back compartment, pry bar resting
  on the `+X` end wall with its hook outside (gives the `+X` bound), flat file on the front floor.
- Layout is written in Unreal centimeters (`U()` converts to Blender meters).
- About 1,650 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Paint_Blue` | new instance of `M_Prop_Base`: painted blue, BaseColor `(0.055, 0.27, 0.40)`, Roughness `0.55`, GrimeAmount `0.4` |
| `Steel_Tools` | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` |
| `Grip_Yellow` | new instance of `M_Prop_Base`: yellow plastic grips, BaseColor `(0.62, 0.47, 0.13)`, Roughness `0.40` |
| `Rubber_Black` | new instance of `M_Prop_Base`: black rubber, BaseColor `(0.018, 0.018, 0.020)`, Roughness `0.70` |

## Unreal

- Asset: `/Game/Objects/Toolbox/SM_Toolbox` (Nanite off, 1 convex hull collision).
- Used by `/Game/Objects/Interactable/ToolBox_BP` (replaces `SM_Toolbox_Prestaged_A01_N1`; same pivot, so the Blueprint
  components keep their placement).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
