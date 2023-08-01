#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "reflectionCapture.hlsl"

struct vs_out
{
	float4 pos : SV_Position;
	float3 texCoord : TEX;
};

TextureCube<float4> g_environmentTexture : TEXTURE: register(t20);

cbuffer EnvironmentTextureInfo : register(b10)
{
    int2 g_environmentTextureSize;
};

cbuffer RoughnessValue : register(b11)
{
	float g_roughness;
	float3 pad;
};

float4 main(vs_out input) : SV_Target
{
	float3 hemisphereNormal = normalize(input.texCoord);

	float3x3 rotMatrix = basisFromDir(hemisphereNormal);

	float3 resultColor = float3(0.0, 0.0, 0.0);

	const int SAMPLES_COUNT = 10000;
    const float EPSILON = 0.0001;
    int skippedSamplesCount = 0;
	for (int i = 0; i < SAMPLES_COUNT; i++)
	{
		float NdotH;
		
		float roughness = g_roughness;
		float r4 = pow(roughness, 4);

		float3 h = randomGGX(NdotH, i, SAMPLES_COUNT, r4, rotMatrix);

		float3 l = reflect(-hemisphereNormal, h);

		float S = 4 / (2 * PI * ggx(r4, NdotH) * SAMPLES_COUNT);

        int Mip = 0.5 * log2(S * 3 * g_environmentTextureSize.x * g_environmentTextureSize.y);

		float3 E = g_environmentTexture.SampleLevel(g_pointWrap, l, Mip).rgb;

		float NdotL = dot(hemisphereNormal, l);
        if (NdotL < EPSILON)
        {
            skippedSamplesCount++;
            continue;
        }
		
		resultColor += E;
	}

    resultColor /= (SAMPLES_COUNT - skippedSamplesCount);
	return float4(resultColor, 1.0);
}