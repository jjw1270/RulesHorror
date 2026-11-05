# Computer

The desk computer of `L_Office` as one static mesh, `SM_Computer`: CRT monitor, tower, keyboard, mouse and all
cables (keyboard and mouse cords, the monitor's VGA and power cords, the tower power cord, a power strip on the floor
and a wall outlet). Because everything is one mesh, the cables always meet the devices they belong to.

It is the mesh of the `Monitor` component of `/Game/Objects/Interactable/Computer_BP`. The monitor screen, the UI
widget and the interaction box stay exactly where they were with the separate monitor mesh.

## Files

- `create_computer_blend.py`: generates the `.blend` and the FBX. Run it again after changing it, and also after
  changing the monitor (`../ComputerMonitor`) or the tower, keyboard or mouse (`../RetroComputer`): the parts are
  appended from `ComputerMonitor.blend` and `RetroComputer.blend`.
- `Computer.blend`: Blender source. The scene contains `SM_Computer` and its four `UCX_SM_Computer_##` collision hulls.
- `Exports/SM_Computer.fbx`: FBX for Unreal import (mesh and collision hulls).

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_computer_blend.py
```

Append `-- --preview <dir>` to load the desk, the mouse pad and the room at their `L_Office` positions, print the
clearance of every cable to the parts, the level meshes and the other cables (`CLEARANCE ...` lines, should not be
negative) and render previews (seat, top, behind, under, mouse, keyboard). The reference meshes are not saved.

## Layout

- Frame: the `Monitor` component frame (= `SM_ComputerMonitor`): pivot at the bottom center of the monitor stand,
  screen facing Unreal `+Y`. In `Computer_BP` the `Monitor` component sits at the BP root with yaw 90.
- Parts are placed like the computer in `L_Office` (`Computer_BP` local cm, yaw): monitor `(0, 0, 0)` 90, tower
  `(0, -42, 0)` 90, keyboard `(-53.175, -0.091, 0)` 90 (centered in front of the monitor), mouse
  `(-43.399, 44.106, 0.843)` 85 (on top of the level's mouse pad `SM_ComputerMousePad_A01_N1`, which stays a separate
  level actor). In `SM_Computer` coordinates: tower at `x = -42`, keyboard at `y = 53.2`, mouse at `(44.1, 43.4)`.
- Cables: the script writes the cable points in `L_Office` world coordinates of a design frame that puts the actor at
  `(328, 268, -42)` (`W()`); the real actor is at `(328.44, 267.67, -41.84)` with yaw 90, so the desk top, mouse pad
  and wall are taken from their real positions relative to the actor. The keyboard and mouse cables start inside the
  devices (points given in the device frame, `kb()` / `ms()`).
- Keyboard (beige, r 0.25 cm): from the back of the keyboard through the gap between monitor and tower, behind the
  tower and up into a PS/2 plug. Mouse (r 0.22 cm): out of the mouse nose, over the back edge of the mouse pad, past
  the monitor on the mouse side, along the back edge of the desk to the PS/2 port above the keyboard plug.
- Monitor VGA (grey, r 0.42 cm): from the rear shell over the desk edge, hangs in a loop behind the desk, up to a
  VGA plug in the graphics card slot. Monitor and tower power (black, r 0.36 cm): down behind the desk into a
  4-socket power strip on the floor (lit red switch); the strip cord goes to a wall outlet on the `+Y` wall.
- The floor and wall parts assume the `L_Office` desk (83 cm, against the wall). Unreal bounds (cm)
  `(-52.5, -37.3, -83.2) .. (47.2, 56.4, 45.7)`, about 28,500 triangles.

## Material slots

Slot order is fixed: the faces are stored sorted by slot, so Unreal gets this order. Slot 1 must stay `Screen`
(C++ `AComputer` calls `SetMaterial(1, screenMID)`).

| # | Slot | Used by | Unreal material |
|---|---|---|---|
| 0 | `Monitor_Plastic` | monitor | `/Game/Objects/ComputerMonitor/MI_ComputerMonitor_Plastic_Beige` |
| 1 | `Screen` | monitor | `/Game/Objects/Materials/MI_CRTScreen_Office` (replaced at runtime by a dynamic instance) |
| 2 | `Monitor_Dark` | monitor | `/Game/Objects/ComputerMonitor/MI_ComputerMonitor_Plastic_Dark` |
| 3 | `Plastic_Beige` | tower, keyboard, plugs, power strip | `/Game/Objects/RetroComputer/MI_RetroComputer_Plastic_Beige` |
| 4 | `Panel_Dark` | tower, VGA plug, sockets | `/Game/Objects/RetroComputer/MI_RetroComputer_Panel_Dark` |
| 5 | `LED` | tower | `/Game/Objects/RetroComputer/MI_RetroComputer_LED` |
| 6 | `Keys_Light` | keyboard | `/Game/Objects/RetroComputer/MI_RetroComputer_Keys_Light` |
| 7 | `Keys_Dark` | keyboard | `/Game/Objects/RetroComputer/MI_RetroComputer_Keys_Dark` |
| 8 | `Plastic_OffWhite` | mouse, wall outlet | `/Game/Objects/RetroComputer/MI_RetroComputer_Plastic_OffWhite` |
| 9 | `Seam_Dark` | mouse | `/Game/Objects/RetroComputer/MI_RetroComputer_Seam_Dark` |
| 10 | `Cable_Beige` | keyboard and mouse cables | `MI_Computer_Cable_Beige` (`0.58, 0.55, 0.47`, roughness 0.55) |
| 11 | `Cable_Black` | power cords, plugs | `MI_Computer_Cable_Black` (`0.018`, roughness 0.45) |
| 12 | `Cable_Grey` | VGA cable | `MI_Computer_Cable_Grey` (`0.11, 0.11, 0.115`, roughness 0.5) |
| 13 | `Switch_Red` | power strip switch | `MI_Computer_Switch_Red` (emissive `1.1, 0.012, 0.004`) |
| 14 | `Metal` | VGA thumb screws | `/Game/Objects/Materials/MI_Prop_Steel_Brushed` |

The `MI_Computer_*` instances are in `/Game/Objects/Computer` (parent `M_Prop_Base`).

## Unreal

- Asset: `/Game/Objects/Computer/SM_Computer` (Nanite off). Collision: the four `UCX_` convex hulls from the FBX
  (monitor, tower, keyboard, mouse; each through the extreme points of the part); the cables have none.
- `Computer_BP`: component `Monitor` (C++ `MonitorMesh`, collision `BlockAllDynamic`) uses `SM_Computer`;
  `ScreenWidget` and `InteractBox` are attached to it. There are no separate tower, keyboard, mouse or cable components.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`,
  Blender `+Y` as Unreal `-Y`.
