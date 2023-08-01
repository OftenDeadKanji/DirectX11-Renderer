#include "../globals.hlsl"

struct vs_out
{
	float4 worldPos : SV_POSITION;
	float2 texCoord : TEX;
	float time : TIME;
};

struct gs_out
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEX;
	float time : TIME;
	uint renderTargetIndex : SV_RenderTargetArrayIndex;
	float4 fragPos : POS;
};

cbuffer PerCubemap : register(b10)
{
	int g_cubemapIndex;
};

[maxvertexcount(18)]
void main(
	triangle vs_out input[3],
	inout TriangleStream< gs_out > output
)
{
	for (int i = 0; i < 6; i++)
	{
		for (uint j = 0; j < 3; j++)
		{
			gs_out element;
			element.fragPos = input[j].worldPos;
			element.pos = mul(input[j].worldPos, g_pointLights[g_cubemapIndex].depthViewProj[i]);
			element.texCoord = input[j].texCoord;
			element.time = input[j].time;
			element.renderTargetIndex = g_cubemapIndex * 6 + i;
			output.Append(element);
		}
		output.RestartStrip();
	}
}