#include "../globals.hlsl"

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float3 worldPos : POS;
};

struct gs_out
{
	float4 position_clip : SV_POSITION;
	float3 worldPos : POS;
	float3 normal : NORM;
};

[maxvertexcount(3)]
void main(
	triangle vs_out input[3] : SV_POSITION,
	inout LineStream< gs_out > output
)
{
	float3 v0 = input[0].worldPos;
	float3 v1 = input[1].worldPos;
	float3 v2 = input[2].worldPos;

	float3 v01 = v0 - v1;
	float3 v21 = v2 - v1;

	float3 triangleNormal = normalize(cross(v21, v01));

	float3 center = (v0 + v1 + v2) / 3.0;

	gs_out begin, end;
	
	begin.worldPos = center;
	begin.position_clip = mul(float4(begin.worldPos, 1.0), g_viewProj);
	begin.normal = triangleNormal;

	output.Append(begin);

	end.worldPos = center + 0.025 * triangleNormal;
	end.position_clip = mul(float4(end.worldPos, 1.0), g_viewProj);
	end.normal = triangleNormal;

	output.Append(end);

	output.RestartStrip();
}