#include "../globals.hlsl"

struct vs_in
{
	float3 position : POS;
	float4 color : COL;
	float rotationAngle : ANG;
	float2 size : SIZE;
	uint frameIndex : FIDX;
	float frameFraction : FFRAC;

	uint vertexID : SV_VertexID;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float3 worldPos : POS;
	float3x3 basis : BS;
	uint frameIndex : FIDX;
	float frameFraction : FFRAC;
	float4 color : COL;
	float2 texCoord : TEX;
};

static const float2 VERTICES_SIZE_MULTIPLIER[4] =
{
    { -1.0,  1.0 },
    { -1.0, -1.0 },
    {  1.0,  1.0 },
    {  1.0, -1.0 }
};
static const float2 VERTICES_TEX_COORD[4] =
{
    { 0.0, 0.0 },
    { 0.0, 1.0 },
    { 1.0, 0.0 },
    { 1.0, 1.0 }
};

vs_out main(vs_in input)
{
	vs_out output;

	float4 posWS = float4(0.0, 0.0, 0.0, 1.0);
    posWS.xy += input.size * VERTICES_SIZE_MULTIPLIER[input.vertexID];
    
	output.texCoord = VERTICES_TEX_COORD[input.vertexID];
	
	float4x4 localRot = {
		cos(input.rotationAngle), -sin(input.rotationAngle), 0, 0,
		sin(input.rotationAngle), cos(input.rotationAngle), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	float4 cameraBehind = { 0.0f, 0.0f, -1.0, 0.0 };
	cameraBehind = mul(cameraBehind, g_viewInv);

	float3 zAxis = normalize(cameraBehind.xyz);
	float3 xAxis = normalize(cross(float3(0.0, 1.0, 0.0), zAxis));
	float3 yAxis = cross(zAxis, xAxis);

	float4x4 billboardMatrix = {
		xAxis, 0.0,
		yAxis, 0.0,
		zAxis, 0.0,
		input.position, 1.0
	};

	float4x4 modelMatrix = mul(localRot, billboardMatrix);
	output.basis = float3x3( modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz );
	
	posWS = mul(posWS, modelMatrix);

	output.worldPos = posWS;
	output.position = mul(posWS, g_viewProj);
	output.color = input.color;

	output.frameIndex = input.frameIndex;
	output.frameFraction = input.frameFraction;

	return output;
}