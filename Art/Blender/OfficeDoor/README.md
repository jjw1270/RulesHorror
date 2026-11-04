# OfficeDoor

Self-made office door slab for RulesHorror: off-white painted steel door with a narrow framed window
(frosted glass above a recessed solid panel), a pull handle on each face and two hinges on the room side.
Replaces the purchased `BrutalistLevelKit` `SM_Door_Basic3`, which cannot be pushed to the public repository.

## Files

- `create_office_door_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `OfficeDoor.blend`: Blender source. The scene contains only `SM_OfficeDoor`.
- `Exports/SM_OfficeDoor.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_office_door_blend.py
```

Append `-- --preview <dir>` to also render a thumbnail-angle preview and a room-side (`-Y`) preview (not saved into the `.blend`).

## Geometry

- Size: `1.01 x 0.24 x 1.96 m` (Unreal bounds x 0 .. 101.13, y -8.75 .. 14.86, z 4.33 .. 200 cm), same as the original within 0.03 cm.
- Pivot: same as the original: hinge edge (`x = 0`) at floor level, center of the 4.4 cm slab thickness (`y = 0`).
  The slab starts 4.33 cm above the pivot (floor gap); the free (handle) edge is at `x = 101.13`.
- Faces: Unreal `+Y` (Blender `-Y`) is the outside face with the deep pull handle (reaches `y = 14.86`);
  Unreal `-Y` (Blender `+Y`) is the room side with the shallow pull handle (reaches `y = -8.75`) and the hinges.
- Parts: slab with softened edges and a window cut-out, metal window frame (through-going lip plus a wider moulding
  on the room side and a slimmer one outside), painted glazing panel around the frosted glass (z 99 .. 163 cm),
  recessed painted panel below it, two pull handles (round bar on two posts with rosettes and screw caps, z 63.5 .. 120 cm,
  centered at x = 92 cm), two butt hinges (leaf, knuckle, screw heads) at z 173 .. 193 and 44 .. 63.5 cm.
- About 2,700 triangles, no textures (box-projected UVs, 1 UV unit = 1 m).
- The `L_Office` door actor is placed with scale `(-1, 1, 1)`, so in the level the mesh appears mirrored
  (seen from the room, the hinges are on the left and the handle on the right).

## Material slots

| Slot | Unreal material |
|---|---|
| `Paint` | new `M_Prop_Base` instance: BaseColor `(0.62, 0.62, 0.60)`, Metallic 0, Roughness 0.55, GrimeAmount 0.35 (slab, glazing panel, lower panel) |
| `Metal` | new `M_Prop_Base` instance: BaseColor `(0.45, 0.45, 0.46)`, Metallic 1, Roughness 0.4, GrimeAmount 0.3 (window frame, handles, hinges) |
| `Glass` | `MI_OfficeDoor_Glass` (`M_Prop_Glass`): BaseColor `(0.035, 0.05, 0.045)`, Opacity 0.55, Roughness 0.35. A light base color made the pane look milky white in the level, so it is dark glass that mostly shows the dark corridor and reflections. |

Base colors are linear.

## Unreal

- Asset: `/Game/Objects/Door/SM_OfficeDoor`.
- Used by `/Game/Objects/Door/BP_OfficeDoor` in `L_Office` (the room's only door; static, it never opens).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
