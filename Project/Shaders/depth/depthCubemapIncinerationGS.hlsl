#include "../globals.hlsl"

struct vs_out
{
    float4 worldPos : SV_Position;
    float2 texCoord : TEX;
    float3 spherePosition : SPHP;
    float sphereBigRadius : SPHBR;
    float sphereSmallRadius : SPHSR;
};

struct gs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
    float3 spherePosition : SPHP;
    float sphereBigRadius : SPHBR;
    float sphereSmallRadius : SPHSR;
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
	inout TriangleStream<gs_out> output
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
            element.spherePosition = input[j].spherePosition;
            element.sphereBigRadius = input[j].sphereBigRadius;
            element.sphereSmallRadius = input[j].sphereSmallRadius;
            element.renderTargetIndex = g_cubemapIndex * 6 + i;
            output.Append(element);
        }
        output.RestartStrip();
    }
}