#include "../globals.hlsl"
#include "fog.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

cbuffer FogParams : register(b10)
{
    float absorptionCoefficient;
    float scatteringCoefficient;
    float phaseFunctionParameter;
};
    
Texture2D g_sceneColor : TEXTURE : register(t20);
Texture2D g_sceneDepth : TEXTURE : register(t21);

float3 calculateWorldPosition(float2 screenPosition, float depth)
{
    float2 xy = screenPosition / float2(g_gbufferTexturesSize.x, g_gbufferTexturesSize.y);
    xy.y = 1.0 - xy.y;
    xy = (xy - 0.5) * 2.0;
    float4 posCS = float4(xy, depth, 1.0);
    float4 posWS = mul(posCS, g_viewProjInv);
    
    return posWS.xyz / posWS.w;
}

float4 main(vs_out input) : SV_TARGET
{
    float3 sceneColor = g_sceneColor.Sample(g_pointWrap, input.texCoord).rgb;
    float sceneDepth = g_sceneDepth.Sample(g_pointWrap, input.texCoord).r;
    float3 worldPos = calculateWorldPosition(input.pos.xy, sceneDepth);
    
    float3 dirLightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
    float3 toDirLightDirection = -g_directionalLight.direction;
    float3 viewDirection = getViewDirection(worldPos);
    
    float d = g_zNear * g_zFar / (g_zFar + sceneDepth * (g_zNear - g_zFar));
    float cosTheta = dot(toDirLightDirection, viewDirection);
    
    float3 colorWithFog = calculateFog(sceneColor, dirLightEnergy, d, g_fogDensity, g_phaseFunctionParameter, cosTheta);
    
    return float4(colorWithFog, 1.0f);
}