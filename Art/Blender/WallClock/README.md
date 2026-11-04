# WallClock

Self-made round office wall clock for RulesHorror: a body (wood ring, white dial, numerals, glass) and three separate hands.
Replaces the purchased `MannDev` clock meshes, which cannot be pushed to the public repository.

| New mesh | Replaces |
|---|---|
| `SM_WallClock` | `/Game/Assets/MannDev/Assets/Clock/Merged/SM_Merged_Clock` |
| `SM_WallClock_HourHand` | `/Game/Assets/MannDev/Assets/Clock/SeperatedMesh/SM_Object_15` |
| `SM_WallClock_MinuteHand` | `/Game/Assets/MannDev/Assets/Clock/SeperatedMesh/SM_Object_17` |
| `SM_WallClock_SecondHand` | `/Game/Assets/MannDev/Assets/Clock/SeperatedMesh/SM_Object_21` |

## Files

- `create_wall_clock_blend.py`: generates the `.blend` and all four FBX files. Run it again after changing it.
- `WallClock.blend`: Blender source. The scene contains only the four meshes, all at the origin (shared pivot).
- `Exports/<SM_Name>.fbx`: one FBX per mesh for Unreal import.

Regenerate without opening Blender:

```
"C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python create_wall_clock_blend.py
```

Append `-- --preview <dir>` to also render previews after the `.blend` is saved: the body at the thumbnail angle,
each hand from the front, and the whole clock posed at 10:08:30 (front and thumbnail angle; the posed views leave
out the glass, which Workbench draws as a heavy haze).

## Geometry

Unreal bounds in cm (Blender `+Y` = Unreal `-Y`). All four match the originals to 0.01 cm.

| Mesh | Unreal bounds min .. max (cm) | Tris | Parts |
|---|---|---|---|
| `SM_WallClock` | (-18.47, -0.84, -18.47) .. (18.47, 1.02, 18.47) | 6,060 | Wood ring (outer radius 18.47, inner 16.4, rounded profile, 80 segments), white dial disc (face at y = -0.50, also closes the back), 60 minute-track ticks (bold at the hours), numerals 1-12, flat glass disc (y = 0.80 .. 0.86) held in the ring's inner lip. |
| `SM_WallClock_HourHand` | (-0.60, -0.418, -2.588) .. (0.60, -0.267, 7.885) | 104 | Hub (radius 0.6), tapered pointed blade, short tail. |
| `SM_WallClock_MinuteHand` | (-0.60, -0.267, -2.588) .. (0.60, -0.115, 12.138) | 104 | Hub, slimmer tapered pointed blade, short tail. |
| `SM_WallClock_SecondHand` | (-0.60, -0.115, -2.588) .. (0.60, 0.036, 15.697) | 268 | Domed center cap (radius 0.6) that hides the hub stack, thin needle, tail with a round counterweight. |

- Numerals are real geometry made from Blender's built-in font (`Bfont Regular`): flat glyph fill, outline offset
  0.03 for a bold look, extruded in the script into closed 0.4 mm slabs whose face sits 0.3 mm in front of the dial.
  Every `1` gets a small flag stroke (the built-in `1` is a bare stem, which made `11` read like `II`).
  Two-digit numerals use a tighter digit gap than the font's spacing. Digit height 2.4 cm, centers on a 13.7 cm circle.
- No textures; box-projected UVs (1 UV unit = 1 m). All parts are closed shells with outward normals.

## Pivot and rotation contract (C++ `AClock`)

- All four meshes share one pivot: the clock center, at the origin. Do not move it.
- The front (dial, glass) faces Unreal `+Y`; the back (wall side) is Unreal `-Y` at y = -0.84.
- At rotation 0 every hand points to Unreal `+Z` (12 o'clock). `AClock` turns each hand component with
  `FRotator(pitch, 0, 0)` (about Unreal Y through the origin), pitch = -30 deg/hour, -6 deg/min, -6 deg/sec.
  Negative pitch turns a hand toward Unreal `+X`, which is 3 o'clock as seen from the front (`+Y` side):
  3 o'clock = `+X`, 9 = `-X`, 12 = `+Z`, 6 = `-Z`.
- The hands sit in separate Y layers so they never intersect: hour -0.418 .. -0.267, minute -0.267 .. -0.115,
  second -0.115 .. 0.036 (the dial face is at -0.50, the glass at 0.80).

## Material slots

All slots except `Glass` are meant for `/Game/Objects/Materials/M_Prop_Base` instances (parameters: `BaseColor`,
`Metallic`, `Roughness`, `GrimeAmount`); `Glass` is a `/Game/Objects/Materials/M_Prop_Glass` instance.

| Slot | Mesh | Base color (linear) | Rough | Unreal material |
|---|---|---|---|---|
| `Wood` | body | 0.11, 0.06, 0.035 | 0.5 | `MI_WallClock_Wood` (opaque, grime 0.3) |
| `Dial` | body | 0.80, 0.80, 0.78 | 0.6 | `MI_WallClock_Dial` (opaque, grime 0.15) |
| `Ink` | body (numerals, ticks) | 0.02, 0.02, 0.02 | 0.5 | `MI_WallClock_Ink` (opaque) |
| `Glass` | body | 0.9, 0.95, 0.95 | 0.05 | `MI_WallClock_Glass` (glass). In Unreal: BaseColor 0.02, Opacity 0.08 — a light translucent color washed the black numerals to grey. |
| `Hand` | all three hands | 0.015, 0.015, 0.015 | 0.4 | `MI_WallClock_Hand` (opaque, grime 0), shared |

## Unreal

- Assets: `/Game/Objects/Clock/<SM_Name>` (Nanite off).
- Tick sounds: `Art/Audio/ClockTick` (`/Game/Objects/Clock/Sounds`).
- Used by `/Game/Objects/Clock_BP` (C++ `AClock`, `Source/RulesHorror/Objects/Clock.cpp`): the body component
  (`Base`) uses `SM_WallClock`; the C++ components `Hour`, `Minute`, `Second` use the three hands.
- Import settings: FBX exported with forward `-Z`, up `Y`, unit scale applied. Blender `+X` arrives as Unreal `+X`.
