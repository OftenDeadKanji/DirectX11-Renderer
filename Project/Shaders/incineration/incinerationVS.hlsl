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
    float4 sphere : INS_SPH;
    float3 particleColor : INS_PCOL;
    float spherePreviousBigRadius : INS_SPHP;
    float sphereSmallRadius : INS_SPHR;
    uint instanceNumber : INS_NUMBER;
    
    uint vertexID : SV_VertexID;
};

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3x3 TBN : TBN;
    float3 fragPos : POS;
    float3 color : COL;
    float3 normal : NORM;
    float2 texCoord : TEX;
    float3 spherePosition : SPHP;
    float sphereBigRadius : SPHBR;
    float sphereSmallRadius : SPHSR;
    float3 particleColor : PCOL;
    uint instanceNumber : INS_NUMBER;
};

struct ParticleInstance
{
    float4 color;
    float3 position;
    float lifetime;
    float3 speed;
    float size;
};
RWStructuredBuffer<ParticleInstance> g_particleDataBuffer : SBUF : register(u5);
RWBuffer<uint> g_particlesRangeBuffer : BUFF : register(u6);
/*
	0 - number;
	1 - offset;
	2 - expired;

	3 - indexCountPerInstance;
	4 - instanceCount;
	5 - startIndexLocation;
	6 - baseVertexLocation;
	7 - startInstanceLocation;
*/

static const float GPU_PARTICLE_INITIAL_SIZE = 0.2;

void spawnGPUParticle(float3 position, float3 particleColor, float3 speed)
{
    unsigned int particleIndex;
    InterlockedAdd(g_particlesRangeBuffer[0], 1, particleIndex);
    
    if (particleIndex >= MAX_GPU_PARTICLES)
    {
        InterlockedAdd(g_particlesRangeBuffer[0], -1);
    }
    else
    {
        unsigned int offset = g_particlesRangeBuffer[1];
            
        particleIndex = (particleIndex + offset) % MAX_GPU_PARTICLES;
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
            g_particleDataBuffer[particleIndex].position = position + g_cameraPosition;
#else
        g_particleDataBuffer[particleIndex].position = position;
#endif
        g_particleDataBuffer[particleIndex].color = float4(particleColor, 1.0);
        g_particleDataBuffer[particleIndex].speed = speed;
        g_particleDataBuffer[particleIndex].lifetime = 0.0;
        g_particleDataBuffer[particleIndex].size = GPU_PARTICLE_INITIAL_SIZE;
    }
}

vs_out main(vs_in input)
{
    vs_out output = (vs_out) 0;

    float4x4 modelToWorld = float4x4(input.instanceMat0, input.instanceMat1, input.instanceMat2, input.instanceMat3);

    float4 pos = float4(input.position_local, 1.0);
    float4 worldPos = mul(pos, modelToWorld);

    output.fragPos = worldPos.xyz;

    output.position_clip = mul(worldPos, g_viewProj);

    output.color = input.color_local;

    float3 axisX = normalize(input.instanceMat0.xyz);
    float3 axisY = normalize(input.instanceMat1.xyz);
    float3 axisZ = normalize(input.instanceMat2.xyz);

    float3 N = normalize(input.normal.x * axisX + input.normal.y * axisY + input.normal.z * axisZ);
    float3 T = normalize(input.tangent.x * axisX + input.tangent.y * axisY + input.tangent.z * axisZ);
    float3 B = cross(T, N);

    float3x3 TBN = float3x3(T, B, N);

    output.TBN = TBN;
    output.normal = N;

    output.texCoord = input.textureCoordinates;
    output.spherePosition = input.sphere.xyz;
    output.sphereBigRadius = input.sphere.w;
    output.sphereSmallRadius = input.sphereSmallRadius;
    output.particleColor = input.particleColor;
    output.instanceNumber = input.instanceNumber;

    float distanceToSphereCenter = distance(worldPos.xyz, input.sphere.xyz);
    const uint PARTICLE_SPAWN_RATE_LIMITER = 3;
    if (input.vertexID % PARTICLE_SPAWN_RATE_LIMITER == 0 && distanceToSphereCenter <= input.sphere.w && distanceToSphereCenter >= input.spherePreviousBigRadius)
    {
        spawnGPUParticle(worldPos.xyz, input.particleColor, input.normal);
    }
    
    return output;
}