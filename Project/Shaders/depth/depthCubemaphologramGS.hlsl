#include "../globals.hlsl"
#include "distortion.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 frag_pos : POS0;
    float3 frag_pos2 : POS1;
    float3 normal : NORM;
};

struct gs_out
{
	float4 pos : SV_POSITION;
	uint renderTargetIndex : SV_RenderTargetArrayIndex;
};

cbuffer PerCubemap : register(b10)
{
	int g_cubemapIndex;
};

[maxvertexcount(18)]
void main(
	triangle vs_out input[3] : SV_POSITION,
	inout TriangleStream< gs_out > output
)
{
	float3 v0 = input[0].frag_pos;
	float3 v1 = input[1].frag_pos;
	float3 v2 = input[2].frag_pos;

	float3 v01 = v0 - v1;
	float3 v21 = v2 - v1;

	float3 triangleNormal = normalize(cross(v21, v01));

	float3 center = (v0 + v1 + v2) / 3.0;
	float3 offset = vertexDistortion(center, triangleNormal);

	for (int i = 0; i < 6; i++)
	{
		for (uint j = 0; j < 3; j++)
		{
			gs_out element;

			float3 pos = input[j].frag_pos2 + offset;
			element.pos = mul(float4(pos, 1.0), g_pointLights[g_cubemapIndex].depthViewProj[i]);

			element.renderTargetIndex = g_cubemapIndex * 6 + i;
			output.Append(element);
		}
		output.RestartStrip();
	}
}