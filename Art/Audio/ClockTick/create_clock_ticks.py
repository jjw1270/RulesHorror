"""Synthesizes the wall clock tick sounds for RulesHorror (replaces the purchased MannDev clock ticks).

Each file is a 1 s, 44.1 kHz, 16-bit stereo WAV: a short mechanical tick (sharp click + a few damped metal/plastic
resonances + a faint second hit from the escapement) followed by a quiet small-room tail. Ten variations with small
random differences so the per-second tick does not sound like a loop.

Run:  python create_clock_ticks.py      (needs numpy)
Output: Exports/SW_ClockTick_01.wav .. SW_ClockTick_10.wav
"""
import wave
from pathlib import Path

import numpy as np

SR = 44100
LENGTH = 1.0
COUNT = 10
PEAK = 0.38  # about -8.4 dBFS
OUT = Path(__file__).resolve().parent / "Exports"


def damped_sine(t, freq, tau, phase=0.0):
    return np.sin(2.0 * np.pi * freq * t + phase) * np.exp(-t / tau)


def hit(rng, n, strength, bright):
    """One mechanical impact: filtered noise click + damped resonances."""
    t = np.arange(n) / SR
    noise = rng.standard_normal(n)
    click = np.diff(noise, prepend=0.0) * np.exp(-t / (0.00045 + 0.0002 * rng.random()))
    modes = [
        (950.0, 0.012, 0.25),   # plastic case body
        (1900.0, 0.009, 0.55),  # movement housing
        (3300.0, 0.006, 0.45),  # small gear / pawl
        (5400.0, 0.003, 0.30),  # metallic edge
    ]
    out = click * 0.6 * bright
    for f, tau, amp in modes:
        f *= 1.0 + rng.uniform(-0.04, 0.04)
        tau *= 1.0 + rng.uniform(-0.2, 0.2)
        out += amp * damped_sine(t, f, tau, rng.uniform(0, 2 * np.pi))
    return out * strength


def room_ir(rng, n, rt60):
    t = np.arange(n) / SR
    ir = rng.standard_normal(n) * np.exp(-6.91 * t / rt60)
    ir[: int(0.003 * SR)] = 0.0  # first reflections arrive after ~3 ms
    # gentle low-pass so the tail is dull like a furnished room
    k = 6
    ir = np.convolve(ir, np.ones(k) / k, mode="same")
    return ir / np.sqrt(np.sum(ir ** 2))


def make_tick(seed):
    rng = np.random.default_rng(seed)
    n = int(SR * LENGTH)
    dry = np.zeros(n)
    hit_len = int(0.06 * SR)
    start = int(0.004 * SR)
    dry[start:start + hit_len] += hit(rng, hit_len, 1.0, rng.uniform(0.8, 1.2))
    # escapement second hit, 6-14 ms later and much weaker
    d2 = start + int(rng.uniform(0.006, 0.014) * SR)
    dry[d2:d2 + hit_len] += hit(rng, hit_len, rng.uniform(0.18, 0.32), 0.7)
    stereo = []
    for ch in range(2):
        ir = room_ir(rng, int(0.45 * SR), rng.uniform(0.22, 0.30))
        size = n + len(ir) - 1  # FFT convolution (np.convolve takes about a minute here)
        wet = np.fft.irfft(np.fft.rfft(dry, size) * np.fft.rfft(ir, size), size)[:n]
        stereo.append(dry + wet * 0.12)
    sig = np.stack(stereo, axis=1)
    # fade the tail to silence
    fade = int(0.1 * SR)
    sig[-fade:] *= np.linspace(1.0, 0.0, fade)[:, None]
    sig *= PEAK / np.max(np.abs(sig)) * rng.uniform(0.85, 1.0)
    return sig


def write_wav(path, sig):
    data = np.clip(sig, -1.0, 1.0)
    pcm = (data * 32767.0).astype("<i2")
    with wave.open(str(path), "wb") as w:
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for i in range(COUNT):
        path = OUT / f"SW_ClockTick_{i + 1:02d}.wav"
        write_wav(path, make_tick(1000 + i))
        print("wrote", path)


if __name__ == "__main__":
    main()
