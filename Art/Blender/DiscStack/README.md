# DiscStack

Self-made messy stack of CD jewel cases with a loose disc on top for RulesHorror.
Replaces the purchased `Deko_MatrixDemo` `SM_Record_Stack_C01_N1`, which cannot be pushed to the public repository.

## Files

- `create_disc_stack_blend.py`: generates the `.blend`, the texture and the FBX. Run it again after changing it.
- `DiscStack.blend`: Blender source. The scene contains only `SM_DiscStack`.
- `Textures/T_DiscStack_Print.png`: 1024 x 1024 procedural atlas (CD label + three abstract case inserts, no text or logos).
- `Exports/SM_DiscStack.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_disc_stack_blend.py
```

## Geometry

- Size: `0.17 x 0.16 x 0.06 m`, same bounds as the original (Unreal X -8.2..8.7, Y -8.5..7.5, Z 0.1..6.1 cm).
- Pivot: same as the original, under the middle of the stack, `Z` up; the bottom case starts 1 mm above the pivot.
- Parts: five standard jewel cases (14.2 x 12.5 x 1.04 cm), each with a translucent shell, black disc tray, hinge strip and
  (except the top one) a printed insert; slightly turned and tilted against each other. The top case has a ribbed hinge,
  and a loose silver CD (label ring printed, hub and data side silver) lies on it.
- The stack layout is written in Unreal coordinates in `CASES` at the top of the script.
- About 1,300 triangles. Printed faces use the atlas UVs (0-1); all other faces are box-projected.

## Material slots

| Slot | Unreal material |
|---|---|
| `Case_Clear` | glass instance: BaseColor `(0.55, 0.58, 0.62)`, Opacity `0.15`, Roughness `0.05` |
| `Tray_Black` | new instance of `M_Prop_Base`: black plastic, BaseColor `(0.012, 0.012, 0.014)`, Roughness `0.35` |
| `Print_Atlas` | textured instance with `T_DiscStack_Print` (tint `1,1,1`), Roughness `0.45` |
| `Disc_Silver` | new instance of `M_Prop_Base`: BaseColor `(0.85, 0.86, 0.88)`, Metallic `1`, Roughness `0.12` |

## Unreal

- Asset: `/Game/Objects/DiscStack/SM_DiscStack` (Nanite off, 1 convex hull collision).
- Replaces `SM_Record_Stack_C01_N1` in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
