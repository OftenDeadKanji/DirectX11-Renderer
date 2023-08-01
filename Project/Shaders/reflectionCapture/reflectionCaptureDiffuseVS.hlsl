#include "../globals.hlsl"

struct vs_out
{
	float4 pos : SV_POSITION;
	float3 texCoord : TEX;
};

// to avoid using if-else
static const float2 TRIANGLE_VERTEX_POS[3] = {
	float2(-1.0, -1.0),
	float2(-1.0,  3.0),
	float2(3.0, -1.0)
};

static const float2 CORNERS_FACTORS[3] = {
	float2(0.0, 0.0),
	float2(0.0, 2.0),
	float2(2.0, 0.0)
};

vs_out main(uint id: SV_VertexID)
{
	vs_out output;

	output.pos = float4(TRIANGLE_VERTEX_POS[id], 0.0, 1.0);
	output.texCoord = g_FrustumBLdir + g_FrustumBRdir * CORNERS_FACTORS[id].x + g_FrustumTLdir * CORNERS_FACTORS[id].y;

	return output;
}