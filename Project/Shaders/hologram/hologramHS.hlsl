struct vs_out
{
	float4 position_clip : SV_POSITION;
	float3 frag_pos : POS0;
	float3 frag_pos2 : POS1;
	float3 normal : NORM;
	nointerpolation float3 color : COL;
    uint instanceNumber : INS_NUMBER;
};

struct PatchOut
{
	float EdgeFactors[3] : SV_TessFactor;
	float InsideFactor : SV_InsideTessFactor;
};

static const float SUBTRIANGLE_SIZE = 10.0;

PatchOut mainPatch(
	InputPatch<vs_out, 3> input,
	uint PatchID : SV_PrimitiveID)
{
	int size0 = ceil(distance(input[0].frag_pos, input[1].frag_pos) * SUBTRIANGLE_SIZE);
	int size1 = ceil(distance(input[1].frag_pos, input[2].frag_pos) * SUBTRIANGLE_SIZE);
	int size2 = ceil(distance(input[2].frag_pos, input[0].frag_pos) * SUBTRIANGLE_SIZE);

	PatchOut output;
	output.EdgeFactors[0] = size0;
	output.EdgeFactors[1] = size1;
	output.EdgeFactors[2] = size2;
	output.InsideFactor = ceil((size0 + size1 + size2) / 3.0);

	return output;
}

[outputcontrolpoints(3)]
[domain("tri")]
[outputtopology("triangle_cw")]
[partitioning("integer")]
[patchconstantfunc("mainPatch")]
vs_out main(
	InputPatch<vs_out, 3> input,
	uint pointID : SV_OutputControlPointID,
	uint patchID : SV_PrimitiveID )
{
	return input[pointID];
}
