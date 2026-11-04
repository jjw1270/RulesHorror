# ClockTick

Self-made wall clock tick sounds for RulesHorror.
Replaces the purchased `MannDev` clock ticks (`SW_clocktick_*`), which cannot be pushed to the public repository.

## Files

- `create_clock_ticks.py`: synthesizes the sounds with numpy (no recordings). Run it again after changing it.
- `Exports/SW_ClockTick_01.wav` .. `SW_ClockTick_10.wav`: 1 s, 44.1 kHz, 16-bit stereo.

Regenerate:

```
python create_clock_ticks.py
```

## Sound

Each tick is a short filtered-noise click plus four damped resonances (about 0.95, 1.9, 3.3 and 5.4 kHz: case,
movement, gear, metal edge), a weaker second hit 6-14 ms later (escapement), and a quiet small-room tail.
The ten files use different random seeds, so the per-second tick does not sound like a loop. Peak about -8 dBFS.

## Unreal

- Assets: `/Game/Objects/Clock/Sounds/SW_ClockTick_01` .. `SW_ClockTick_10` (SoundWave, imported from `Exports/`).
- Used by `/Game/Objects/Clock_BP` (`_ClockTickingSounds`). `AClock` plays a random one every second.
- To reimport after regenerating: right-click the sound assets in the Content Browser and choose Reimport.
