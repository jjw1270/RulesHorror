# OfficeTrashCan

Self-made office pedal trash can for RulesHorror.
Replaces the purchased `BrutalistLevelKit` `SM_Trash_Can`, which cannot be pushed to the public repository.

## Files

- `create_office_trash_can_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `OfficeTrashCan.blend`: Blender source. The scene contains only `SM_OfficeTrashCan`.
- `Exports/SM_OfficeTrashCan.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_trash_can_blend.py
```

## Geometry

- Size: `0.50 x 0.51 x 0.90 m`, same footprint as the original so placed actors keep their transform.
- Pivot: bottom center of the body. `+X` = front (pedal side), `Z` up.
- Parts: metal foot band, painted body, metal top rim, plastic lid, rear hinge, front pedal, front label plate.
- About 2,900 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Body_Paint` | `/Game/Objects/Materials/MI_Prop_PaintedMetal_Navy` |
| `Trim_Metal` | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` |
| `Lid_Plastic` | `/Game/Objects/Materials/MI_Prop_Plastic_DarkBlue` |

All three are instances of `/Game/Objects/Materials/M_Prop_Base` (parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`). `GrimeAmount` adds world-space noise to darken the color and raise roughness slightly.

## Unreal

- Asset: `/Game/Objects/TrashCan/SM_OfficeTrashCan` (Nanite off, 1 convex hull collision).
- Used in `L_Office` (`StaticMeshActor_34`).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
