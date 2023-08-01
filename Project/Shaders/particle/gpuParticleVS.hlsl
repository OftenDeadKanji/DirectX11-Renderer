#include "../globals.hlsl"

struct vs_out
{
    float4 position : SV_POSITION;
    float3 worldPos : POS;
    float3x3 basis : BS;
    float4 color : COL;
    float2 texCoord : TEX;
};
struct ParticleInstance
{
    float4 color;
    float3 position;
    float lifetime;
    float3 speed;
    float size;
};
StructuredBuffer<ParticleInstance> g_particleDataBuffer : SBUF : register(t20);
Buffer<uint> g_particlesRangeBuffer : BUFF : register(t21);
/*
	0 - number;
	1 - offset;
	2 - expired;

    billboards arguments
	3 - indexCountPerInstance;
	4 - instanceCount;
	5 - startIndexLocation;
	6 - baseVertexLocation;
	7 - startInstanceLocation;

    spheres arguments
    8  - indexCountPerInstance;
	9  - instanceCount;
	10 - startIndexLocation;
	11 - baseVertexLocation;
	12 - startInstanceLocation;
*/

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

vs_out main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    vs_out output;

    uint offset = g_particlesRangeBuffer[1];
    uint particleIndex = (instanceID + offset) % MAX_GPU_PARTICLES;
    ParticleInstance particle = g_particleDataBuffer[particleIndex];
    
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    particle.position -= g_cameraPosition;
#endif
    
    float4 posWS = float4(0.0, 0.0, 0.0, 1.0);
    posWS.xy += particle.size * VERTICES_SIZE_MULTIPLIER[vertexID];
    
    output.texCoord = VERTICES_TEX_COORD[vertexID];

    float4 cameraBehind = { 0.0f, 0.0f, -1.0, 0.0 };
    cameraBehind = mul(cameraBehind, g_viewInv);

    float3 zAxis = normalize(cameraBehind.xyz);
    float3 xAxis = normalize(particle.speed);
    float3 yAxis = cross(zAxis, xAxis);

    float4x4 billboardMatrix =
    {
        xAxis, 0.0,
		yAxis, 0.0,
		zAxis, 0.0,
		particle.position, 1.0
    };

    float4x4 modelMatrix = billboardMatrix;
    output.basis = float3x3(modelMatrix[0].xyz, modelMatrix[1].xyz, modelMatrix[2].xyz);
	
    posWS = mul(posWS, modelMatrix);

    output.worldPos = posWS;
    output.position = mul(posWS, g_viewProj);
    output.color = particle.color;

    return output;
}