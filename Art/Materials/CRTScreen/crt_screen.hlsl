// CRT screen effect for M_CRTScreen (Custom node body, output Float3 -> Emissive Color).
// Inputs: UV (float2), Time, Tex (texture object "Display Input"), FilterOn, Intensity, ScanCount, ScanInt,
//         MaskInt, MaskScale, Curv, Vig, Flick, Noise, RollInt, RollSpeed, Glow.
// The monitor mesh maps UV 0..1 exactly onto the in-game UI widget, so Curvature is kept small:
// edge midpoints stay put, corners are cropped slightly, the center is magnified slightly.
float2 cc = UV - 0.5;
float r2 = dot(cc, cc);
float2 uv = 0.5 + cc * (1.0 + Curv * (r2 * 4.0 - 1.0));
float inside = step(0.0, uv.x) * step(uv.x, 1.0) * step(0.0, uv.y) * step(uv.y, 1.0);

// image + soft phosphor glow (4 diagonal taps ~1.5 widget pixels away)
float3 c = Texture2DSample(Tex, TexSampler, uv).rgb;
float2 o = float2(1.5 / 928.0, 1.5 / 786.0);
float3 g = Texture2DSample(Tex, TexSampler, uv + float2(o.x, o.y)).rgb
         + Texture2DSample(Tex, TexSampler, uv + float2(-o.x, o.y)).rgb
         + Texture2DSample(Tex, TexSampler, uv + float2(o.x, -o.y)).rgb
         + Texture2DSample(Tex, TexSampler, uv + float2(-o.x, -o.y)).rgb;
c = c + g * 0.25 * Glow;

// scanlines, faded out when they get smaller than ~1 screen pixel (avoids moire at a distance)
float sl = uv.y * ScanCount;
float s = 0.5 + 0.5 * cos(sl * 6.2831853);
float scan = lerp(1.0, 1.0 - ScanInt * (1.0 - s), saturate(2.0 - fwidth(sl) * 2.0));

// RGB aperture grille, same fade
float mx = uv.x * MaskScale * 3.0;
float ph = frac(mx / 3.0) * 3.0;
float3 stripe = float3(ph < 1.0 ? 1.0 : 0.0, (ph >= 1.0 && ph < 2.0) ? 1.0 : 0.0, ph >= 2.0 ? 1.0 : 0.0);
float3 grille = lerp(float3(1.0 - MaskInt, 1.0 - MaskInt, 1.0 - MaskInt), float3(1.0, 1.0, 1.0), stripe);
float3 mask = lerp(float3(1.0, 1.0, 1.0), grille, saturate(2.0 - fwidth(mx) * 2.0));

// vignette, slow brightness flicker, rolling bar, fine noise
float v = pow(saturate(16.0 * uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y)), Vig);
float fl = 1.0 + Flick * (sin(Time * 7.3) * 0.6 + sin(Time * 23.1) * 0.4);
float roll = frac(uv.y * 0.5 + Time * RollSpeed);
float bar = 1.0 + RollInt * pow(sin(roll * 3.14159265), 12.0);
float2 cell = floor(uv * float2(928.0, 786.0)) + floor(Time * 24.0) * float2(17.0, 31.0);
float n = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);

float3 col = c * scan * mask * v * fl * bar + (n - 0.5) * Noise * v;
return max(col, float3(0.0, 0.0, 0.0)) * Intensity * FilterOn * inside;
