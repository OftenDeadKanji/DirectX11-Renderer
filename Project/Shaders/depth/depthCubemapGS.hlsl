#include "../globals.hlsl"

struct gs_out
{
	float4 pos : SV_POSITION;
	uint renderTargetIndex : SV_RenderTargetArrayIndex;
	float4 fragPos : POS;
};

cbuffer PerCubemap : register(b10)
{
	int g_cubemapIndex;
};

[maxvertexcount(18)]
void main(
	triangle float4 worldPos[3] : SV_POSITION, 
	inout TriangleStream< gs_out > output
)
{
	for (int i = 0; i < 6; i++)
	{
		for (uint j = 0; j < 3; j++)
		{
			gs_out element;
			element.fragPos = worldPos[j];
			element.pos = mul(worldPos[j], g_pointLights[g_cubemapIndex].depthViewProj[i]);
			element.renderTargetIndex = g_cubemapIndex * 6 + i;
			output.Append(element);
		}
		output.RestartStrip();
	}
}