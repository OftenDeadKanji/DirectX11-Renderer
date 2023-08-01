#include "../globals.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

cbuffer BloomDownsampleInfo : register(b10)
{
    int2 g_previousMipTextureSize;
};

Texture2D g_previousMipTexture : TEXTURE : register(t20);

float4 main(vs_out input) : SV_TARGET
{
    float2 texelSize = 1.0 / g_previousMipTextureSize;
    float x = texelSize.x;
    float y = texelSize.y;
    
    float3 a = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-2 * x,  2 * y)).rgb;
    float3 b = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0,      2 * y)).rgb;
    float3 c = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 2 * x,  2 * y)).rgb;
                                                                                            
    float3 d = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-2 * x,  0    )).rgb;
    float3 e = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0    ,  0    )).rgb;
    float3 f = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 2 * x,  0    )).rgb;
                                                                                            
    float3 g = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-2 * x, -2 * y)).rgb;
    float3 h = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 0,     -2 * y)).rgb;
    float3 i = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( 2 * x, -2 * y)).rgb;
    
    float3 j = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-x, +y)).rgb;
    float3 k = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( x, +y)).rgb;
    float3 l = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2(-x, -y)).rgb;
    float3 m = g_previousMipTexture.Sample(g_bilinearClamp, input.texCoord + float2( x, -y)).rgb;
    
    float3 outColor = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
    
    return float4(outColor, 1.0);
}