struct gs_out
{
	float4 position_clip : SV_POSITION;
	float3 worldPos : POS;
	float3 normal : NORM;
};

float4 main(gs_out input) : SV_TARGET
{
	float3 normal = (normalize(input.normal) + 1.0) * 0.5;
	return float4(normal, 1.0);
}