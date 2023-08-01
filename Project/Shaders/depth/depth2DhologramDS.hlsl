struct vs_out
{
	float4 position_clip : SV_POSITION;
	float3 frag_pos : POS0;
	float3 frag_pos2 : POS1;
	float3 normal : NORM;
};

struct PatchOut
{
	float EdgeFactors[3] : SV_TessFactor;
	float InsideFactor : SV_InsideTessFactor;
};

[domain("tri")]
vs_out main(PatchOut control, float3 loc : SV_DomainLocation, const OutputPatch<vs_out, 3> input)
{
	vs_out output;
	
	output.position_clip	= loc.x * input[0].position_clip	+ loc.y * input[1].position_clip	+ loc.z * input[2].position_clip;
	output.frag_pos			= loc.x * input[0].frag_pos			+ loc.y * input[1].frag_pos			+ loc.z * input[2].frag_pos;
	output.frag_pos2		= loc.x * input[0].frag_pos2		+ loc.y * input[1].frag_pos2		+ loc.z * input[2].frag_pos2;
	output.normal			= loc.x * input[0].normal			+ loc.y * input[1].normal			+ loc.z * input[2].normal;

	return output;
}
