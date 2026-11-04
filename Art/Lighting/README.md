# Lighting

Self-made light profiles for RulesHorror.

## Files

- `create_ies_profiles.py`: generates the IES files (plain Python 3, no dependencies).
- `IES/T_IES_Downlight.ies`: soft downlight profile.

Regenerate:

```
python create_ies_profiles.py
```

## T_IES_Downlight

- Replaces the purchased `BrutalistLevelKit` `T_IES1` used by the `L_Lobby` spotlight.
- Rotationally symmetric, lights only downward: a bright narrow beam (about 10° spread), a soft spill, and a bright wide arc just below the horizon (around 74°) that lights nearby walls.
- 850 lm total (the spotlight does not use IES brightness, so only the shape matters).
- Unreal asset: `/Game/Objects/Lights/T_IES_Downlight` (TextureLightProfile).
