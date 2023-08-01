#include "../globals.hlsl"

struct vs_out
{
	float4 pos : SV_Position;
	float3 texCoord : TEX;
};

TextureCube<float4> g_skyTexture : TEXTURE : register(t20);

float4 main(vs_out input) : SV_Target
{
	return g_skyTexture.Sample(g_bilinearWrap, normalize(input.texCoord));
}