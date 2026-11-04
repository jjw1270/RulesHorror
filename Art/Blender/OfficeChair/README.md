# OfficeChair

Self-made swivel office chair for RulesHorror: green fabric seat and tall backrest on a black plastic shell,
flat chrome arms with black pads, gas lift, chrome 5-star base with twin-wheel casters.
Replaces the purchased `BrutalistLevelKit` `SM_Office_Chair`, which cannot be pushed to the public repository.

## Files

- `create_office_chair_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `OfficeChair.blend`: Blender source. The scene contains only `SM_OfficeChair`.
- `Exports/SM_OfficeChair.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_chair_blend.py
```

Append `-- --preview <dir>` to also render a thumbnail-angle preview (not saved into the `.blend`).

## Geometry

- Size: `0.75 x 0.74 x 1.30 m` (Unreal bounds x -37.7 .. 37.7, y -40.7 .. 33.1, z 0 .. 129.7 cm), same as the original within 1 cm.
- Pivot: same as the original: floor level under the gas lift. The seat faces Unreal `+Y` (Blender `-Y`), the backrest is on the
  Unreal `-Y` side. One leg of the 5-star base points backwards (Unreal `-Y`), which gives the asymmetric Y bounds.
- Parts: 5 tapered chrome legs, chrome-hooded twin casters, hub, gas lift (black shroud + chrome piston), tilt mechanism with lever,
  seat cushion with waterfall front on a black shell, tapered backrest (fabric front, black shell rim, 13 degree recline) with a
  lumbar bracket and spine, flat chrome arms turned slightly outward with black pads.
- About 7,000 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Fabric_Green` | new `M_Prop_Base` instance: BaseColor `(0.09, 0.2, 0.14)`, Metallic 0, Roughness 0.9, GrimeAmount 0.35 |
| `Plastic_Black` | new `M_Prop_Base` instance: BaseColor `(0.02, 0.02, 0.022)`, Metallic 0, Roughness 0.45, GrimeAmount 0.3 |
| `Metal_Chrome` | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` |

Base colors are linear. All are instances of `/Game/Objects/Materials/M_Prop_Base`.

## Unreal

- Asset: `/Game/Objects/OfficeChair/SM_OfficeChair`.
- The original is used in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
