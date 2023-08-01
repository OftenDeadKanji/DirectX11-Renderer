#include "../globals.hlsl"
#include "../brdf.hlsl"

struct gs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
    float3 spherePosition : SPHP;
    float sphereBigRadius : SPHBR;
    float sphereSmallRadius : SPHSR;
    uint renderTargetIndex : SV_RenderTargetArrayIndex;
    float4 fragPos : POS;
};

Texture2D g_noiseTex : TEXTURE : register(t23);

float4 main(gs_out input) : SV_TARGET
{
    //float threshold = input.time;
    //float noise = g_noiseTex.Sample(g_pointWrap, input.texCoord);
    //
    //if (noise >= threshold)
    //{
    //    return float4(0.0, 0.0, 0.0, 0.0);
    //}

    float distanceToSphereCenter = distance(input.fragPos.xyz, input.spherePosition);
    float effectValue = (distanceToSphereCenter - input.sphereSmallRadius) / (input.sphereBigRadius - input.sphereSmallRadius);
    
    float alpha = 1.0;
    if (effectValue > 1.0)
    {
    }
    if (effectValue >= 0.0 && effectValue <= 1.0)
    {
        alpha = g_noiseTex.Sample(g_pointWrap, input.texCoord);
    }
    else if (effectValue <= 0.0)
    {
        alpha = 0.0;
    }
    
    float threshold = 0.2;
    float falloff = 0.1;
    float dx = ddx(input.texCoord);
    float dy = ddy(input.texCoord);
    
    alpha = saturate(((alpha - threshold) / (abs(dx * alpha) + abs(dy * alpha))) / falloff);
    
    return float4(0.0, 0.0, 0.0, alpha);
}