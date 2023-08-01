#include "../globals.hlsl"
#include "../brdf.hlsl"

struct gs_out
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEX;
	float time : TIME;
	uint renderTargetIndex : SV_RenderTargetArrayIndex;
	float4 fragPos : POS;
};

Texture2D g_noiseTex : TEXTURE: register(t23);

float4 main(gs_out input) : SV_TARGET
{
	float threshold = input.time;
	float noise = g_noiseTex.Sample(g_pointWrap, input.texCoord);

	if (noise >= threshold)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}
	
	return float4(0.0, 0.0, 0.0, 1.0);
}