#ifndef __FOG_HLSL__
#define __FOG_HLSL__
#include "../globals.hlsl"

float phaseFunction(float cosTheta, float g)
{
    float oneOver4PI = 1.0 / (4.0 * PI);
    return oneOver4PI * (1 - g * g) / pow(1 + g * g + 2 * g * cosTheta, 3.0 / 2.0);
}

// cosTheta = cos of angle between -dirLightDirection and viewDirection
float3 calculateFog(float3 sceneColor, float3 dirLightEnergy, float distance, float density, float phaseFunctionParameter, float cosTheta)
{
    float F_ex = exp(-density * distance);
    float3 L_ex = sceneColor * F_ex;
    
    float phaseFunctionVal = phaseFunction(cosTheta, phaseFunctionParameter);
    
    float3 L_in = (1.0 / max(density, 0.0001)) * dirLightEnergy * density * min(phaseFunctionVal, 1.0) * (1.0 - exp(-density * distance));
    
    float3 L = L_ex + L_in;
    return L;
}

#endif
