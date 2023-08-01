struct vs_out
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEX;
};

// to avoid using if-else
static const float2 TRIANGLE_VERTEX_POS[3] = {
	float2(-1.0, -1.0),
	float2(-1.0,  3.0),
	float2(3.0, -1.0)
};

static const float2 TRIANGLE_VERTEX_TEX_COORDS[3] = {
	float2(0.0, 1.0),
	float2(0.0, -1.0),
	float2(2.0, 1.0)
};

vs_out main(uint id: SV_VertexID)
{
	vs_out output;

	output.pos = float4(TRIANGLE_VERTEX_POS[id], 0.0, 1.0);
	output.texCoord = TRIANGLE_VERTEX_TEX_COORDS[id];

	return output;
}