#include "../globals.hlsl"

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float4 color : COL;
	float2 texCoord : TEX;
    uint instanceNumber : INS_NUMBER;
};

Texture2D g_diffuse : TEXTURE : register(t20);

cbuffer Material : register(b10)
{
	bool usesDiffuseTexture;
};

float4 main(vs_out input) : SV_TARGET
{
	return usesDiffuseTexture ? g_diffuse.Sample(g_bilinearWrap, input.texCoord) : input.color;
}