#include "../globals.hlsl"
#include "../brdf.hlsl"

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float2 texCoord : TEX;
	float time : TIME;
};

Texture2D g_noiseTex : TEXTURE: register(t23);

float4 main(vs_out input) : SV_TARGET
{
	float threshold = input.time;
	float noise = g_noiseTex.Sample(g_pointWrap, input.texCoord);

	if (noise >= threshold)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	return float4(0.0, 0.0, 0.0, 1.0);
}
