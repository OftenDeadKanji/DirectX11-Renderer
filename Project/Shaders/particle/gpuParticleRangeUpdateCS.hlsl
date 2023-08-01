#include "../globals.hlsl"

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

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint number = g_particlesRangeBuffer[0];
    uint offset = g_particlesRangeBuffer[1];
    uint expired = g_particlesRangeBuffer[2];
    
    offset = (offset + expired) % (MAX_GPU_PARTICLES);
    number -= expired;
    expired = 0;
    
    g_particlesRangeBuffer[0] = number;
    g_particlesRangeBuffer[1] = offset;
    g_particlesRangeBuffer[2] = expired;
    g_particlesRangeBuffer[4] = number;
    g_particlesRangeBuffer[9] = number;
}