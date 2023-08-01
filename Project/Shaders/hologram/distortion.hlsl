#ifndef __DISTORTION_HLSL__
#define __DISTORTION_HLSL__
#include "../globals.hlsl"

// BEGIN ShaderToy https://www.shadertoy.com/view/WttcRB
float hash4d(in float4 p) {
    p = frac(p * 0.1031);
    p += dot(p, p.zwyx + 31.32);
    return frac((p.x + p.y) * p.z - p.x * p.w);
}

float noise4d(in float4 p) {
    float4 cell = floor(p);
    float4 local = frac(p);
    local *= local * (3.0 - 2.0 * local);

    float ldbq = hash4d(cell);
    float rdbq = hash4d(cell + float4(1.0, 0.0, 0.0, 0.0));
    float ldfq = hash4d(cell + float4(0.0, 0.0, 1.0, 0.0));
    float rdfq = hash4d(cell + float4(1.0, 0.0, 1.0, 0.0));
    float lubq = hash4d(cell + float4(0.0, 1.0, 0.0, 0.0));
    float rubq = hash4d(cell + float4(1.0, 1.0, 0.0, 0.0));
    float lufq = hash4d(cell + float4(0.0, 1.0, 1.0, 0.0));
    float rufq = hash4d(cell + float4(1.0, 1.0, 1.0, 0.0));
    float ldbw = hash4d(cell + float4(0.0, 0.0, 0.0, 1.0));
    float rdbw = hash4d(cell + float4(1.0, 0.0, 0.0, 1.0));
    float ldfw = hash4d(cell + float4(0.0, 0.0, 1.0, 1.0));
    float rdfw = hash4d(cell + float4(1.0, 0.0, 1.0, 1.0));
    float lubw = hash4d(cell + float4(0.0, 1.0, 0.0, 1.0));
    float rubw = hash4d(cell + float4(1.0, 1.0, 0.0, 1.0));
    float lufw = hash4d(cell + float4(0.0, 1.0, 1.0, 1.0));
    float rufw = hash4d(cell + 1.0);

    return lerp(lerp(lerp(lerp(ldbq, rdbq, local.x),
        lerp(lubq, rubq, local.x),
        local.y),

        lerp(lerp(ldfq, rdfq, local.x),
            lerp(lufq, rufq, local.x),
            local.y),

        local.z),

        lerp(lerp(lerp(ldbw, rdbw, local.x),
            lerp(lubw, rubw, local.x),
            local.y),

            lerp(lerp(ldfw, rdfw, local.x),
                lerp(lufw, rufw, local.x),
                local.y),

            local.z),

        local.w);
}

float noise4d(in float4 p, uniform in float octaves)
{
    float nscale = 1.0;
    float tscale = 0.0;
    float value = 0.0;

    for (float octave = 0.0; octave < octaves; octave++) {
        value += noise4d(p) * nscale;
        tscale += nscale;
        nscale *= 0.5;
        p *= 2.0;
    }

    return value / tscale;
}
// END ShaderToy https://www.shadertoy.com/view/WttcRB

float distanceIntensity(float value, float target, float fade)
{
    return min(1.0, abs(value - target) / fade);
}

float periodIntensity(float value, float period, float fade)
{
    float target = round(value / period) * period;
    return distanceIntensity(value, target, fade);
}

float wave(float3 pos, float waveInterval, float waveYSpeed, float waveThickness, uniform bool distort)
{
    if (distort)
    {
        const float WAVE_XZ_SPEED = 3.0;
        const float WAVE_DISTORTION_SIZE = 0.035;
        const float WAVE_OSCILLATING_TIME = 4;

        float distortionSign = abs(frac(g_time / WAVE_OSCILLATING_TIME) - 0.5) * 4 - 1;
        float2 distortion = sin(pos.xz / WAVE_DISTORTION_SIZE + g_time * WAVE_XZ_SPEED) * WAVE_DISTORTION_SIZE * distortionSign;
        pos.y -= (distortion.x + distortion.y);
    }

    pos.y -= g_time * waveYSpeed;

    float intensity = 1.0 - periodIntensity(pos.y, waveInterval, waveThickness);
    return intensity;
}

static const float BLUE_WAVE_INTERVAL = 0.8;
static const float BLUE_WAVE_SPEED = 0.25;
static const float BLUE_WAVE_THICKNESS = 0.05;

static const float RED_WAVE_INTERVAL = 10;
static const float RED_WAVE_SPEED = 2;
static const float RED_WAVE_THICKNESS = 0.2;

float3 vertexDistortion(float3 pos, float3 normal)
{
    float3 offset = 0.0;
    offset += normal * 0.025 * wave(pos, BLUE_WAVE_INTERVAL, BLUE_WAVE_SPEED, BLUE_WAVE_THICKNESS, true);
    offset += normal * 0.05 * wave(pos, RED_WAVE_INTERVAL, RED_WAVE_SPEED, RED_WAVE_THICKNESS, false);
    return offset;
}


// called in pixel shader
float3 colorDistortion(float3 pos, float3 normal, float3 color, float3 toCamera)
{
    float blueWave = wave(pos, BLUE_WAVE_INTERVAL, BLUE_WAVE_SPEED, BLUE_WAVE_THICKNESS, true);
    float redWave = wave(pos, RED_WAVE_INTERVAL, RED_WAVE_SPEED, RED_WAVE_THICKNESS, false);

    //float3 toCamera = normalize(cameraPosInModelSpace - pos);
    float contourGlow = pow(1.0 - abs(dot(normal, toCamera)), 2);

    // when contourInterference is 0, ripple effect contourWave is added to contourGlow, otherwise contourWave is 1
    float contourWave = wave(pos, 0.1, 0.1, 0.05, false);
    float contourInterference = periodIntensity(g_time, 4, 1);
    contourWave = lerp(contourWave, 1.0, contourInterference);
    // when contourWave is 0.0, contourGlow becomes darker, otherwise contourGlow color is plain, without ripple
    contourGlow = lerp(contourGlow / 10, contourGlow, contourWave);

    color *= min(1, contourGlow + blueWave * 0.5);
    float colorNoise = sqrt(noise4d(float4(pos, frac(g_time)) * 100, 1));
    color *= lerp(colorNoise, 1.0, contourInterference);

    color = lerp(color, float3(1.0, 0.0, 0.0), redWave * 0.25);
    return color;
}

#endif