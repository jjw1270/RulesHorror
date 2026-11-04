# CRTScreen

Self-made CRT screen material for the player's office computer.
Replaces the purchased `CRT_VCR_Material` preset (`CRT_Preset_Lobby`), which cannot be pushed to the public repository.

## Files

- `crt_screen.hlsl`: body of the material's Custom node (all screen effects).
- `build_m_crtscreen.py`: builds `M_CRTScreen` and `MI_CRTScreen_Office` in the running editor through the Unreal MCP server
  (`ProgrammaticToolset.execute_tool_script`). It deletes and recreates the material's nodes, so edit the HLSL or the
  parameter defaults here and run it again.

Rebuild (editor open, MCP server on `127.0.0.1:8000`):

1. Copy `crt_screen.hlsl` to `Saved/ClaudeTmp/crt_screen_hlsl.txt` (the MCP file reader only accepts `.txt`/`.json`/... under `Content/` and `Saved/`).
2. Run `build_m_crtscreen.py` as the script of `execute_tool_script`.

## Unreal assets

| Asset | Notes |
|---|---|
| `/Game/Objects/Materials/M_CRTScreen` | Opaque, Default Lit. Base color/roughness/specular = dark glass, emissive = CRT picture. |
| `/Game/Objects/Materials/MI_CRTScreen_Office` | Used by `Computer_BP` (`_ScreenMaterialInstance`). Tune the look here. |

`AComputer` creates a dynamic instance of `_ScreenMaterialInstance`, puts it on material slot 1 of the monitor mesh
(`SM_ComputerMonitor`, slot `Screen`) and sets two parameters:

- `Display Input` (texture): the render target of the `ScreenWidget` widget component (the in-game desktop UI).
- `Filter On` (scalar): 1 when the computer is on, 0 when off. Off shows only the dark glass.

The monitor's screen UVs map 0..1 exactly onto the widget quad, so the picture lines up with the widget's hit-test plane.

## Parameters

| Parameter | Default | Effect |
|---|---|---|
| `Filter On` | 0 | power (set by code) |
| `Emissive Intensity` | 1.0 | picture brightness |
| `Scanline Count` / `Scanline Intensity` | 300 / 0.4 | horizontal scanlines (fade out when smaller than a pixel) |
| `Mask Scale` / `Mask Intensity` | 160 / 0.3 | RGB aperture-grille triads across the width |
| `Curvature` | 0.02 | slight barrel distortion (edge midpoints fixed, corners cropped) |
| `Vignette` | 0.35 | darker edges |
| `Glow` | 0.35 | soft phosphor bloom |
| `Flicker`, `Noise` | 0.02, 0.03 | brightness flicker and fine noise |
| `Roll Bar Intensity` / `Roll Bar Speed` | 0.06 / 0.12 | slow rolling bright band |
| `Glass Color`, `Glass Roughness`, `Glass Specular` | dark green-grey, 0.12, 0.5 | the tube glass when off |
