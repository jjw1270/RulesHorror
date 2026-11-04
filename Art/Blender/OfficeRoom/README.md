# OfficeRoom

Self-made room shell for `L_Office`: walls, floor, ceiling and rubber baseboards in one static mesh.
Replaces the four BSP box brushes (Outliner folder `Room`) that used the engine default checker material.

## Files

- `create_office_room_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `OfficeRoom.blend`: Blender source. The scene contains only `SM_OfficeRoom`.
- `Exports/SM_OfficeRoom.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_room_blend.py
```

Append `-- --preview <dir>` to also render two interior previews (not saved into the `.blend`).

## Geometry

- Same interior as the old BSP room (Unreal cm, origin = room center): `x -395 .. 395`, `y -305 .. 305`, `z -125 .. 125`.
  Walls, floor and ceiling are 10 cm thick (outward), so the outer bounds are `(-405, -315, -135) .. (405, 315, 135)`.
- Door hole in the `+X` wall: `y -200 .. -104`, floor to `z 68` (covered by `BP_OfficeDoor`).
- Window hole in the `-X` wall: `y -223.6 .. 223.6`, `z -10 .. 110`.
- Baseboards: 10 cm high, 1.2 cm thick, along all four walls; on the `+X` wall they stop against the door slab
  (`y -202` and `-101`).
- Every part is a closed box with outward normals (no merged vertices). 272 triangles.
- UVs: box projection, 1 UV unit = 1 m, origin at the room center (the carpet and ceiling textures are laid out for this).

## Material slots

| Slot | Unreal material |
|---|---|
| `Wall` | `MI_OfficeRoom_Wall` (also the hidden outer faces) |
| `Floor` | `MI_OfficeRoom_Carpet` |
| `Ceiling` | `MI_OfficeRoom_Ceiling` |
| `Baseboard` | `MI_OfficeRoom_Baseboard` (`M_Prop_Base`, `0.022, 0.022, 0.024`, roughness 0.55) |

Textures and the normal-mapped material: see `Art/Textures/OfficeRoom/README.md`.

## Unreal

- Asset: `/Game/Objects/OfficeRoom/SM_OfficeRoom` (Nanite off). Collision: complex as simple
  (`CTF_UseComplexAsSimple`, no simple shapes; a convex hull would fill the whole room).
- Placed in `L_Office` at the origin, folder `Room`. The BSP brushes were removed and the BSP rebuilt.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
