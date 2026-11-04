# ComputerCables

Self-made cables for the desk computer in `L_Office`: keyboard and mouse cords, the monitor's VGA and power cords,
the tower power cord, a power strip on the floor and a wall outlet. The purchased computer meshes had no cables.

The computer parts stay separate meshes in `/Game/Objects/Interactable/Computer_BP` (the monitor screen slot and
the UI widget quad must stay exact); the cables are one more static mesh component of the same Blueprint.

## Files

- `create_computer_cables_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `ComputerCables.blend`: Blender source. The scene contains only `SM_ComputerCables`.
- `Exports/SM_ComputerCables.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_computer_cables_blend.py
```

Append `-- --preview <dir>` to load the surrounding meshes (monitor, tower, keyboard, mouse, mouse pad, desk, room)
at their `L_Office` positions, print the clearance of every cable to them and to the other cables
(`CLEARANCE ...` lines, should not be negative) and render four previews. The reference meshes are not saved.

## Layout

- Modeled in `Computer_BP` local space (root on the desk top under the monitor). The actor sits at `L_Office`
  `(328, 268, -42)` with yaw 90, so the script writes every point in level coordinates and converts them (`W()`).
  The component is added at the BP origin with no rotation.
- Keyboard (beige, r 0.25 cm): from the back of the keyboard through the gap between monitor and tower, behind the
  tower and up into a PS/2 plug. Mouse (r 0.22 cm): over the mouse pad edge, past the right side of the monitor,
  along the back edge of the desk to the PS/2 port above the keyboard plug.
- Monitor VGA (grey, r 0.42 cm): from the rear shell over the desk edge, hangs in a loop behind the desk, up to a
  VGA plug in the graphics card slot. Monitor and tower power (black, r 0.36 cm): down behind the desk into a
  4-socket power strip on the floor (lit red switch); the strip cord goes to a wall outlet on the `+Y` wall.
- The floor part assumes the `L_Office` desk (83 cm, against the wall). About 14,900 triangles.

## Material slots

| Slot | Unreal material |
|---|---|
| `Cable_Beige` | `MI_ComputerCables_Cable_Beige` (`0.58, 0.55, 0.47`, roughness 0.55) |
| `Cable_Black` | `MI_ComputerCables_Cable_Black` (`0.018`, roughness 0.45) |
| `Cable_Grey` | `MI_ComputerCables_Cable_Grey` (`0.11, 0.11, 0.115`, roughness 0.5) |
| `Switch_Red` | `MI_ComputerCables_Switch_Red` (emissive `1.1, 0.012, 0.004`) |
| `Plastic_Beige` | `/Game/Objects/RetroComputer/MI_RetroComputer_Plastic_Beige` (plugs, power strip) |
| `Panel_Dark` | `/Game/Objects/RetroComputer/MI_RetroComputer_Panel_Dark` (VGA plug, sockets) |
| `Metal` | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` (thumb screws) |
| `Plastic_OffWhite` | `/Game/Objects/RetroComputer/MI_RetroComputer_Plastic_OffWhite` (wall outlet) |

New instances are in `/Game/Objects/ComputerCables` (parent `M_Prop_Base`).

## Unreal

- Asset: `/Game/Objects/ComputerCables/SM_ComputerCables` (no collision).
- `Computer_BP` component `Cables` (StaticMeshComponent under `Root`, collision `NoCollision`).
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`, Blender `+Y` as Unreal `-Y`.
