# OfficeWorker

Self-made seated office worker for the `L_Office` player, seen only in reflections (the dark CRT glass) and shadows.
The office player (`OfficePawn_BP`, C++ `AOfficePawn`) is a camera without a body; these meshes give it one.

## Files

- `create_office_worker_blend.py`: generates the `.blend` and both FBX files. Run it again after changing it.
- `OfficeWorker.blend`: Blender source (`SM_OfficeWorker_Head`, `SM_OfficeWorker_Body`).
- `Exports/SM_OfficeWorker_Head.fbx`, `Exports/SM_OfficeWorker_Body.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_worker_blend.py
```

Append `-- --preview <dir>` to render front, side, face and back previews (not saved into the `.blend`).

## Geometry

- Both meshes have their origin at the eye point (the pawn camera), Unreal `+X` forward, `+Z` up.
- Shapes are overlapping ellipsoids / capsules per material, fused with a voxel remesh (4.5 mm), smoothed and
  decimated to a triangle budget: soft closed surfaces without seams inside one material.
- `SM_OfficeWorker_Head` (about 5,100 triangles, bounds `(-16.4, -8.9, -29.3) .. (5.8, 9.0, 16.3)` cm): head, hair,
  dark eyes, neck down into the collar. The face surface is 3 to 4 cm in front of the camera.
- `SM_OfficeWorker_Body` (about 10,500 triangles, bounds `(-21.9, -26.7, -120.5) .. (54.3, 26.7, -16.9)` cm): seated pose,
  shirt with long sleeves and a dark tie, hands resting on the thighs, trousers, shoes. Seat about 77 cm and the
  soles about 120 cm below the eye (the `MP_Main` eye is 121 cm above the floor).

## Material slots

| Slot | Unreal material |
|---|---|
| `Shirt` | `/Game/Objects/FPArms/MI_FPArms_Sleeve` (same shirt as the first person arms) |
| `Skin` | `/Game/Objects/FPArms/MI_FPArms_Skin` |
| `Trousers` | `MI_OfficeWorker_Trousers` (`0.045, 0.048, 0.056`, roughness 0.75) |
| `Shoes` | `MI_OfficeWorker_Shoes` (`0.012, 0.011, 0.011`, roughness 0.35) |
| `Tie` | `MI_OfficeWorker_Tie` (`0.09, 0.025, 0.03`, roughness 0.55) |
| `Hair` | `MI_OfficeWorker_Hair` (`0.022, 0.017, 0.013`, roughness 0.6) |
| `Eye` | `MI_OfficeWorker_Eye` (`0.01`, roughness 0.15) |

New instances are in `/Game/Objects/OfficeWorker` (parent `M_Prop_Base`).

## Unreal

- Assets: `/Game/Objects/OfficeWorker/SM_OfficeWorker_Head`, `SM_OfficeWorker_Body` (no collision).
- `AOfficePawn` components (C++): `ReflectionHead` is attached to the camera and turns with the view;
  `ReflectionBody` is attached to the pawn root with absolute rotation, stays upright and follows only 35 % of the
  look yaw (`_BodyYawFollowRatio`). Both use `FirstPersonPrimitiveType = WorldSpaceRepresentation`: hidden from the
  player's own view, still visible in hardware ray traced (Lumen) reflections and shadows, no collision.
  The meshes are assigned in `OfficePawn_BP`.
- When the pawn leans in to `MP_Computer`, the lower body passes through the desk; only the head and shoulders are
  ever reflected in the screen, and the desk hides the rest.
