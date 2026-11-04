# OfficeDesk

Self-made worn wooden office desks for RulesHorror, two variants of the same family.
They replace the purchased `BrutalistLevelKit` desks, which cannot be pushed to the public repository:

| Mesh | Replaces | Layout |
|---|---|---|
| `SM_OfficeDesk_A` | `/Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/SM_Office_Table1_4X` | narrow left pedestal (2 drawers), wide right pedestal (2 drawers, bottom one slightly ajar), open knee space |
| `SM_OfficeDesk_B` | `/Game/Assets/BrutalistLevelKit/Models/Props/Furnitures/SM_Office_Table2_4X` | two equal pedestals (2 drawers each, top-left slightly ajar) and a center drawer under the top |

## Files

- `create_office_desk_blend.py`: generates the `.blend` and both FBX files. Run it again after changing it.
- `OfficeDesk.blend`: Blender source. The scene contains only `SM_OfficeDesk_A` and `SM_OfficeDesk_B` (both at the origin, on top of each other).
- `Exports/SM_OfficeDesk_A.fbx`, `Exports/SM_OfficeDesk_B.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_desk_blend.py
```

Append `-- --preview <dir>` to also render thumbnail-angle previews (not saved into the `.blend`).

## Geometry

- Size: `2.00 x 1.01 x 0.84 m` (Unreal bounds x -99.8 .. 99.8, y 0.9 .. 102.3, z -0.7 .. 83.0 cm), same as the originals within 1 cm
  (the ajar drawer handle sticks out 0.9 cm past the original front bound).
- Pivot: same as the originals: back edge of the top (Unreal `y = 0`), centered in X, floor at `z = 0` (leveling feet go 0.7 cm below).
  Drawer fronts face Unreal `+Y` (Blender `-Y`).
- Parts: 4.5 cm top with softened edges, two end panels to the floor, pedestal carcasses raised on leveling feet,
  inset drawer fronts with loop pulls, modesty panel at the back of the knee space (A also has a slim rail under the front edge).
- About 2,200 (A) / 2,400 (B) triangles, no textures (box-projected UVs, 1 UV unit = 1 m).

## Material slots

| Slot | Unreal material |
|---|---|
| `Wood_Top` | new `M_Prop_Base` instance: BaseColor `(0.30, 0.235, 0.17)`, Metallic 0, Roughness 0.7, GrimeAmount 0.45 (worn, bleached top) |
| `Wood` | new `M_Prop_Base` instance: BaseColor `(0.26, 0.17, 0.10)`, Metallic 0, Roughness 0.65, GrimeAmount 0.35 (drawer fronts) |
| `Frame` | new `M_Prop_Base` instance: BaseColor `(0.10, 0.08, 0.065)`, Metallic 0, Roughness 0.6, GrimeAmount 0.4 (end panels, carcass, rails) |
| `Metal_Handles` | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` (handles, leveling feet) |

Base colors are linear. All are instances of `/Game/Objects/Materials/M_Prop_Base` (parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`).

## Unreal

- Assets: `/Game/Objects/OfficeDesk/SM_OfficeDesk_A`, `/Game/Objects/OfficeDesk/SM_OfficeDesk_B`.
- The originals are used in `L_Office`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
