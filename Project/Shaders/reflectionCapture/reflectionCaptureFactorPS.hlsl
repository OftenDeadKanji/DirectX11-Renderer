#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "reflectionCapture.hlsl"

struct vs_out
{
	float4 pos : SV_Position;
	float2 texCoord : TEX;
};

float4 main(vs_out input) : SV_Target
{
	float NdotV = input.texCoord.x;
	float3 v = { sqrt(1 - NdotV * NdotV), 0, NdotV };

	float roughness = input.texCoord.y;
	float r4 = pow(roughness, 4);

	float3x3 rotMatrix = { 
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0
	};

	const int SAMPLES_COUNT = 10000;
    const float EPSILON = 0.0001;
	
    int skippedSamplesCount = 0;
	float Kr = 0.0, Kg = 0.0;
	for (int i = 0; i < SAMPLES_COUNT; i++)
	{
		float NdotH;
		float3 h = randomGGX(NdotH, i, SAMPLES_COUNT, r4, rotMatrix);

		float3 l = reflect(-v, h);
		float NdotL = l.z;
        if (NdotL < EPSILON)
        {
            skippedSamplesCount++;
            continue;
        }
		
		float G = smith(r4, NdotV, NdotL);

		float HdotV = dot(h, v);
        if (HdotV < EPSILON)
        {
            skippedSamplesCount++;
            continue;
        }
		
		Kr += ((G * (1 - pow(1 - HdotV, 5)) * HdotV) / (NdotV * NdotH));
		Kg += ((G * pow(1 - HdotV, 5) * HdotV) / (NdotV * NdotH));
	}

	Kr /= (SAMPLES_COUNT - skippedSamplesCount);
    Kg /= (SAMPLES_COUNT - skippedSamplesCount);

	return float4(Kr, Kg, 0.0, 1.0);
}