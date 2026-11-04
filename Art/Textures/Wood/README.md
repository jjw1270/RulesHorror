# Wood

Self-made desk wood texture for RulesHorror.

## Files

- `create_wood_texture.py`: generates the texture (Python 3 + numpy + Pillow).
- `T_Wood_Desk_BaseColor.png`: 1024 x 1024 base color, seamless in both directions, grain runs along U.

Regenerate:

```
python create_wood_texture.py
```

## Unreal

- Texture: `/Game/Objects/Materials/T_Wood_Desk_BaseColor`
- `MI_Prop_Wood_Desk` (`M_Prop_Textured`, `UVTiling` 1): for meshes with box-projected UVs (1 UV unit = 1 m), such as the office desks.
- `MI_Prop_Wood_LobbyDesk` (`UVTiling` 4.5): the `L_Lobby` desk-top plane (engine Plane with 0–1 UVs over about 3.2 m).
- Both use a slightly grey tint (0.85, 0.82, 0.8), roughness 0.6 and `GrimeAmount` 0.4.
- Replaces the purchased `BrutalistLevelKit` `MI_Desk_Wood`.
