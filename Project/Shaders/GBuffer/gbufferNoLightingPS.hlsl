#include "../globals.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

Texture2D g_albedo : TEXTURE : register(t20);
Texture2D g_roughness_metalness : TEXTURE : register(t21);
Texture2D g_normal : TEXTURE : register(t22);
Texture2D g_emission : TEXTURE : register(t23);
Texture2D g_objectID : TEXTURE : register(t24);

float4 main(vs_out input) : SV_TARGET
{
    return g_emission.Sample(g_pointWrap, input.texCoord);
}