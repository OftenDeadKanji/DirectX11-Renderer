#include "../globals.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

Texture2D g_previousMipTexture : TEXTURE : register(t20);
Texture2D srcTexture : TEXTURE : register(t21);

float4 main(vs_out input) : SV_TARGET
{
    float3 bloom = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord).rgb;
    float3 src = srcTexture.Sample(g_bilinearClamp, input.texCoord).rgb;
    
    float3 outColor = lerp(src, bloom, 0.04);
    
    return float4(outColor, 1.0);
}