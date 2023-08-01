#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "reflectionCapture.hlsl"

struct vs_out
{
	float4 pos : SV_Position;
	float3 texCoord : TEX;
};

cbuffer EnvironmentTextureInfo : register(b10)
{
    int2 g_environmentTextureSize;
};

TextureCube<float4> g_environmentTexture : TEXTURE: register(t20);

float4 main(vs_out input) : SV_Target
{
	float3 hemisphereNormal = normalize(input.texCoord);
	float3x3 rotMatrix = basisFromDir(hemisphereNormal);

	float3 resultColor = float3(0.0, 0.0, 0.0);

	const int SAMPLES_COUNT = 10000;
	for (int i = 0; i < SAMPLES_COUNT; i++)
	{
		float NdotV;
		float3 hemisphereSample = randomHemisphere(NdotV, i, SAMPLES_COUNT);
		float3 rotatedSample = mul(hemisphereSample, rotMatrix);

		float S = 1.0 / SAMPLES_COUNT;
        uint RES = g_environmentTextureSize.x * g_environmentTextureSize.y;
		float logIn = S * 3.0 * RES;
		int Mip = 0.5 * log2(logIn);

		float3 E = g_environmentTexture.SampleLevel(g_pointWrap, normalize(rotatedSample), Mip).rgb;

		resultColor += ((E * NdotV) / PI) * (1.0 - fresnel(NdotV, float3(0.04, 0.04, 0.04)));
	}

	resultColor *= (2.0 * PI) / SAMPLES_COUNT;
	return float4(resultColor, 1.0);
}