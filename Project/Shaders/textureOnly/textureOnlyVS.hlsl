#include "../globals.hlsl"

struct vs_in
{
	float3 position_local : POS;
	float4 color_local : COL;
	float2 textureCoordinates : TEX;
	float3 normal : NORM;
	float3 tangent : TANG;
	float3 bitangent : BTANG;

	float4 instanceMat0 : INS0;
	float4 instanceMat1 : INS1;
	float4 instanceMat2 : INS2;
	float4 instanceMat3 : INS3;
    uint instanceNumber : INS_NUMBER;
};

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float4 color : COL;
	float2 texCoord : TEX;
    uint instanceNumber : INS_NUMBER;
};

vs_out main(vs_in input)
{
	vs_out output = (vs_out)0;

	float4x4 modelToWorld = float4x4(input.instanceMat0, input.instanceMat1, input.instanceMat2, input.instanceMat3);

	float4 pos = float4(input.position_local, 1.0);

	pos = mul(pos, modelToWorld);
	pos = mul(pos, g_viewProj);

	output.position_clip = pos;
	output.color = input.color_local;
	output.texCoord = input.textureCoordinates;
    output.instanceNumber = input.instanceNumber;
	
	return output;
}