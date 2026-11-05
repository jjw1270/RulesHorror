# ComputerMonitor

Self-made beige early-90s CRT monitor for RulesHorror. This is the player's interactive computer screen: the in-game desktop UI is rendered into a render target and shown on the screen faces through the CRT screen material.
Replaces the purchased `CRT_VCR_Material` `Monitor_r1` (`/Game/Assets/CRT_VCR_Material/Meshes/Monitor_r1`), which cannot be pushed to the public repository.

## Files

- `create_computer_monitor_blend.py`: generates the `.blend` and the FBX. Run it again after changing it.
- `ComputerMonitor.blend`: Blender source. The scene contains only `SM_ComputerMonitor`.
- `Exports/SM_ComputerMonitor.fbx`: FBX for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_computer_monitor_blend.py
```

Append `-- --preview <dir>` to also render previews after the `.blend` is saved: the thumbnail angle, a rear and a side view, a front view with a UV test grid on the screen (red marker must be top-left, arrow pointing up) and a re-import check of the FBX (slot order, screen UVs). The script also asserts the screen contract below on every run.

## Geometry

- Unreal bounds (cm): `(-22.70, -22.44, -0.09) .. (22.70, 22.49, 44.36)`; the original is `(-22.70, -22.41, -0.09) .. (22.70, 22.44, 44.34)`. Placed actors keep their transform.
- Pivot: bottom center of the stand (`z = 0` under the stand, `x` and `y` centered). The screen faces Unreal `+Y` (Blender `-Y`).
- Parts: 45.4 cm front bezel with a chamfered tunnel around the screen, convex CRT glass, front shell with a seam groove, deep rear shell (top nearly level, bottom rising and sides tapering toward the back) with a rounded back, top/side/back vent slots, two back connector blocks, thicker bottom bezel strip with four small dark buttons, a power LED, a power button on the right and a blank dark strip on the left (no text or logos), dark tilt/swivel stand (oval foot and cradle, `z` 0 to about 4 cm visible).
- About 4,500 triangles. All parts are closed shells with outward normals.

## Screen contract (gameplay)

The C++ code and `Computer_BP` rely on these; keep them when editing the script.

| Item | Value |
|---|---|
| Material slot order | `0 = Plastic_Beige`, `1 = Screen` (C++ calls `SetMaterial(1, screenMID)`), `2 = Plastic_Dark`. Faces are also stored sorted by slot, so the FBX lists materials in this order. |
| Screen rectangle | Exactly the UI widget quad, Unreal local `x -18.095 .. 18.095`, `z 11.015 .. 41.665` cm (36.19 x 30.65 cm, center `z = 26.34`). |
| Screen depth | Convex glass, 24 x 20 quads: Unreal `y = 19.0` at the edges and corners, `20.8` in the center. The hidden widget plane is at `y = 18.24`; everything visible inside the rectangle is at `y >= 19.0`. |
| Bezel | Front face at Unreal `y = 22.28` (buttons to 22.49). The chamfered tunnel ends 0.3 to 0.6 mm inside the rectangle edge, so from the front the opening is the screen rectangle. |
| Screen UVs (Unreal) | Planar front projection: `u = (x + 18.095) / 36.19`, `v = (41.665 - z) / 30.65` (`u = 0` at the viewer's left = Unreal `-X`, `v = 0` at the top). |
| Screen UVs (Blender) | Unreal flips V on FBX import, so Blender stores `u = (x + 0.18095) / 0.3619`, `v = (z - 0.11015) / 0.3065` (meters). Only the `Screen` faces use this mapping; all other faces are box-projected (1 UV unit = 1 m). |

## Material slots

| Slot | Base color (linear) | Rough | Unreal material |
|---|---|---|---|
| `Plastic_Beige` (0) | 0.62, 0.58, 0.48 | 0.55 | `M_Prop_Base` instance, `GrimeAmount` 0.35 (same color and roughness as `MI_Prop_Plastic_Beige`) |
| `Screen` (1) | 0.02, 0.025, 0.024 | 0.15 | CRT screen material (set at runtime by `Computer_BP` / C++ as a dynamic instance) |
| `Plastic_Dark` (2) | 0.08, 0.075, 0.07 | 0.5 | `M_Prop_Base` instance (stand, buttons, vents) |

`M_Prop_Base` parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`. `GrimeAmount` adds world-space noise to darken the color and raise roughness slightly.

## Unreal

- Asset: `/Game/Objects/ComputerMonitor/SM_ComputerMonitor` (Nanite off, simple box or convex collision).
- Not placed directly: `../Computer` appends the monitor from `ComputerMonitor.blend` into `SM_Computer`, the mesh of
  `/Game/Objects/Interactable/Computer_BP` component `Monitor`. Run that script again after changing the monitor.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
