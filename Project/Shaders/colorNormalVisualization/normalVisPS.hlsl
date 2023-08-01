#include "../globals.hlsl"

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float3 normal : NORM;
    uint instanceNumber : INS_NUMBER;
};

float4 main(vs_out input) : SV_TARGET
{
	float3 normal = (normalize(input.normal) + 1.0) * 0.5; // [-1, 1] -> [0, 1]
	return float4(normal, 1.0);
}