#include "../globals.hlsl"
#include "distortion.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 frag_pos : POS0;
    float3 frag_pos2 : POS1;
    float3 normal : NORM;
    nointerpolation float3 color : COL;
    uint instanceNumber : INS_NUMBER;
};

struct gs_out
{
    float4 position_clip : SV_POSITION;
};

[maxvertexcount(3)]
void main(
	triangle vs_out input[3],
	inout TriangleStream<gs_out> output
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

    for (uint i = 0; i < 3; i++)
    {
        vs_out element;
        element = input[i];

        float4 pos = float4(element.frag_pos2 + offset, 1.0);
        pos = mul(pos, g_viewProj);

        gs_out outElement;
        outElement.position_clip = pos;
        
        output.Append(outElement);
    }
}