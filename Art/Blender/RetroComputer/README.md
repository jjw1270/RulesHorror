# RetroComputer

Self-made 90s beige computer set for RulesHorror: two keyboards, a CRT monitor, a mouse, a mouse pad and a tower PC.
Replaces the purchased `Deko_MatrixDemo` computer meshes, which cannot be pushed to the public repository.

| New mesh | Replaces |
|---|---|
| `SM_KeyboardErgo` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerKeyboard_A01_N1` |
| `SM_KeyboardStandard` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerKeyboard_A02_N1` |
| `SM_CRTMonitor` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerMonitor_A02_N1` |
| `SM_Mouse` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerMouse_A01_N1` |
| `SM_MousePad` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerMousePad_A01_N1` |
| `SM_PCTower` | `/Game/Assets/Deko_MatrixDemo/Apartment/Meshes/SM_ComputerTower_B01_N1` |

## Files

- `create_retro_computer_blend.py`: generates the `.blend` and all six FBX files. Run it again after changing it.
- `RetroComputer.blend`: Blender source. The scene contains only the six meshes, all at the origin (each keeps its own pivot).
- `Exports/<SM_Name>.fbx`: one FBX per mesh for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_retro_computer_blend.py
```

Append `-- --preview <dir>` to also render a preview PNG per mesh (after the `.blend` is saved).

## Geometry

Unreal bounds in cm (Blender `+Y` = Unreal `-Y`). All match the originals within 0.2 cm, so placed actors
(`L_Office`, `/Game/Objects/Interactable/Computer_BP`) keep their transforms.
The user side is Unreal `+Y`: monitor screen, tower front and keyboard front edges face Unreal `+Y`;
the mouse buttons point to Unreal `-Y` (away from the user).

| Mesh | Unreal bounds min .. max (cm) | Tris | Pivot / facing | Parts |
|---|---|---|---|---|
| `SM_KeyboardErgo` | (-25.0, -12.0, 0.0) .. (25.0, 11.8, 4.0) | 5,836 | bottom, near the center | Split key field (each half turned 10 degrees, gap opening toward the back), tented deck, wavy palm rest, nav cluster, numpad, indicator window. 105 keycaps. |
| `SM_KeyboardStandard` | (-23.3, -14.2, -0.2) .. (23.0, 3.2, 2.6) | 5,080 | bottom, 3.2 cm behind the front edge | Case with raised rim around a key well, raised back strip with indicator window and lenses, 104 keycaps (light alphanumerics, dark modifiers), wedge typing angle (back about 0.8 cm higher). |
| `SM_CRTMonitor` | (-18.5, -19.9, -0.1) .. (18.3, 21.6, 38.0) | 2,228 | bottom, roughly under the middle of the body | Front bezel with rounded opening, curved (cushion) dark glass, seam step to a tapered rear shell and neck box, top vent slots, power button, LED, 4 control buttons, blank badge, tilt/swivel stand. |
| `SM_Mouse` | (-3.1, -5.7, 0.0) .. (3.1, 6.5, 3.2) | 1,310 | bottom center | Egg-shaped domed body (wider at the button end), hump toward the palm, dark seams marking two buttons. |
| `SM_MousePad` | (-14.2, -11.2, 0.0) .. (14.2, 11.2, 1.0) | 572 | bottom center | Rounded rectangle, fabric top on a foam base. |
| `SM_PCTower` | (-10.5, -26.8, 0.0) .. (10.2, 21.2, 45.7) | 2,528 | bottom, 21 cm behind the front panel (like the original) | Steel case, plastic front bezel, CD-ROM bay (tray, eject, LED, jack), second 5.25" drive, 3.5" floppy, power and reset buttons, power/HDD LEDs, lower door panel, intake louvers, feet; back: PSU with grille and expansion slot covers. |

No textures; box-projected UVs (1 UV unit = 1 m). All parts are closed shells with outward normals.

## Material slots

All slots are meant for `/Game/Objects/Materials/M_Prop_Base` instances (parameters: `BaseColor`, `Metallic`, `Roughness`, `GrimeAmount`), except `LED` (emissive family).

| Slot | Used by | Base color (linear) | Rough | Unreal material |
|---|---|---|---|---|
| `Plastic_Beige` | keyboards, monitor, tower | 0.62, 0.58, 0.48 | 0.55 | `MI_Prop_Plastic_Beige` (opaque) |
| `Keys_Light` | keyboards | 0.66, 0.64, 0.58 | 0.45 | `MI_Prop_Keys_Light` (opaque) |
| `Keys_Dark` | keyboards | 0.30, 0.29, 0.27 | 0.45 | `MI_Prop_Keys_Dark` (opaque) |
| `Screen` | monitor | 0.025, 0.034, 0.032 | 0.08 | `MI_Prop_CRT_Screen` (opaque, dark glossy) |
| `Plastic_Grey` | monitor (buttons, badge, vents) | 0.30, 0.29, 0.27 | 0.5 | `MI_Prop_Plastic_Grey` (opaque) |
| `Panel_Dark` | tower (bay gaps, slots, grille backing, feet) | 0.03, 0.03, 0.03 | 0.6 | `MI_Prop_Panel_Dark` (opaque) |
| `LED` | tower | 0.05, 0.45, 0.08, emissive green | 0.3 | `MI_Prop_LED_Green` (emissive, keep it dim) |
| `Plastic_OffWhite` | mouse | 0.70, 0.69, 0.64 | 0.42 | `MI_Prop_Plastic_OffWhite` (opaque) |
| `Seam_Dark` | mouse | 0.04, 0.04, 0.04 | 0.6 | `MI_Prop_Seam_Dark` (opaque) |
| `Pad_Fabric` | mouse pad (top) | 0.58, 0.58, 0.71 | 0.9 | `MI_Prop_Pad_Fabric` (opaque) |
| `Pad_Foam` | mouse pad (sides, bottom) | 0.36, 0.36, 0.43 | 0.85 | `MI_Prop_Pad_Foam` (opaque) |

## Unreal

- Assets: `/Game/Objects/RetroComputer/<SM_Name>` (Nanite off, simple box or convex collision).
- `SM_KeyboardStandard`, `SM_Mouse` and `SM_PCTower` are also used by `/Game/Objects/Interactable/Computer_BP`.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
