#include "../globals.hlsl"

struct vs_in
{
    float3 position_local : POS;
    float4 color_local : COL;
    float2 textureCoordinates : TEX;
    float3 normal : NORM;
    float3 tangent : TANG;
    float3 bitangent : BTANG;
    
    uint instanceID : SV_InstanceID;
};

struct vs_out
{
    float4 positionCS : SV_Position;
    float3 particleCenter : CTR;
    float4 particleColor : COL;
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
*/

static const float GPU_PARTICLE_SPHERE_SIZE_MULTIPLIER = 3.0;

vs_out main(vs_in input)
{
    vs_out output;
    
    uint particleIndex = (input.instanceID + g_particlesRangeBuffer[1]) % MAX_GPU_PARTICLES;
    ParticleInstance particle = g_particleDataBuffer[particleIndex];
    
    float sphereSize = particle.size * GPU_PARTICLE_SPHERE_SIZE_MULTIPLIER;
    
    float4x4 modelMatrix =
    {
        sphereSize, 0, 0, 0,
        0, sphereSize, 0, 0,
        0, 0, sphereSize, 0,
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
        particle.position - g_cameraPosition, 1.0
#else
        particle.position, 1.0
#endif
    };
    
    float4 posWS = mul(float4(input.position_local, 1.0), modelMatrix);
    
    float4 posCS = mul(posWS, g_viewProj);
    output.positionCS = posCS;
    
    output.particleColor = particle.color;
    
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    output.particleCenter = particle.position - g_cameraPosition;
#else
    output.particleCenter = particle.position;
#endif
    
	return output;
}