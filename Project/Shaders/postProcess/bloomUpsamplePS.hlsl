#include "../globals.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};


cbuffer BloomUpsampleInfo : register(b10)
{
    float g_filterRadius;
}

Texture2D g_previousMipTexture : TEXTURE : register(t20);

float4 main(vs_out input) : SV_TARGET
{
    float x = g_filterRadius;
    float y = g_filterRadius;
    
    float3 a = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-x,  y));
    float3 b = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0,  y));
    float3 c = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( x,  y));
                                                                                        
    float3 d = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-x,  0));
    float3 e = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0,  0));
    float3 f = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( x,  0));
                                                                                        
    float3 g = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-x, -y));
    float3 h = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0, -y));
    float3 i = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( x, -y));
    
    float3 outColor = e * 0.4 + (b + d + f + h) * 2.0 + (a + c + g + i);
    outColor *= 1.0 / 16.0;
    
    return float4(outColor, 1.0);

}