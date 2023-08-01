#include "../globals.hlsl"

struct ParticleInstance
{
    float4 color;
    float3 position; // not in camera-centered ws
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
*/

static const float MAX_LIFETIME = 10.0;
static const float SIZE_DECREASE_IN_TIME = 0.01;
static const float3 GRAVITY_ACCELERATION = float3(0.0, -9.81, 0.0);
static const float SPEED_LOSS_MULTIPLIER = 0.5;

Texture2D g_normal : TEXTURE : register(t21);
Texture2D g_objectID : TEXTURE : register(t22);
Texture2D g_sceneDepthTexture : TEXTURE : register(t23);

void updateLifetime(inout ParticleInstance particle);
bool checkCollision(float3 currentPositionWS, float3 nextPositionWS, float particleSize);
void getSceneNormalVectors(float3 positionWS, out float3 microNormal, out float3 macroNormal);

[numthreads(64, 1, 1)]
void main( uint3 globalID : SV_DispatchThreadID, uint flatID : SV_GroupIndex )
{
    if (globalID.x >= g_particlesRangeBuffer[0])
    {
        return;
    }
    
    uint particleIndex = (g_particlesRangeBuffer[1] + globalID.x) % MAX_GPU_PARTICLES;
    ParticleInstance particle = g_particleDataBuffer[particleIndex];
    
    updateLifetime(particle);
   
    float3 nextParticlePos = particle.position + g_deltaTime * particle.speed + GRAVITY_ACCELERATION * pow(g_deltaTime, 2) * 0.5;
    
    bool collision = checkCollision(particle.position, nextParticlePos, particle.size);
    if (collision)
    {
        float3 microNormal, macroNormal;
        getSceneNormalVectors(nextParticlePos, microNormal, macroNormal);
        
        particle.speed = reflect(particle.speed, microNormal);
        particle.speed *= SPEED_LOSS_MULTIPLIER;
    }
    else
    {
        particle.position = nextParticlePos;
    }
        
    float t = particle.lifetime / MAX_LIFETIME;
    particle.color.a = smoothstep(0.3, 0.0, t);
    particle.size -= SIZE_DECREASE_IN_TIME * g_deltaTime;
    particle.speed += GRAVITY_ACCELERATION * g_deltaTime;
    
    g_particleDataBuffer[particleIndex] = particle;
}

void updateLifetime(inout ParticleInstance particle)
{
    particle.lifetime += g_deltaTime;
    
    if (particle.lifetime >= MAX_LIFETIME)
    {
        InterlockedAdd(g_particlesRangeBuffer[2], 1);
    }
}

bool checkCollision(float3 currentPositionWS, float3 nextPositionWS, float particleSize)
{
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float4 currentPosWS = float4(currentPositionWS - g_cameraPosition, 1.0);
    float4 nextPosWS = float4(nextPositionWS - g_cameraPosition, 1.0);
#else
    float4 currentPosWS = float4(currentPositionWS, 1.0);
    float4 nextPosWS = float4(nextPositionWS, 1.0);
#endif
    
    bool collision = false;
    for (float t = 0.0; t <= 1.0; t += 0.05)
    {
        float4 positionWS = lerp(currentPosWS, nextPosWS, t);
        
        float4 positionCS = mul(positionWS, g_viewProj);
        positionCS /= positionCS.w;

        float2 uv = (positionCS.xy + 1) * 0.5;
        uv.y = 1.0 - uv.y;
        
        if (uv.x > 1.0 || uv.x < 0.0 || uv.y > 1.0 || uv.y < 0.0)
        {
            continue;
        }
        
        float sceneDepth = g_sceneDepthTexture.SampleLevel(g_pointWrap, uv, 0);
        float4 scenePosCS = float4(positionCS.xy, sceneDepth, 1.0);
        
        float4 scenePosWS = mul(scenePosCS, g_viewProjInv);
        scenePosWS /= scenePosWS.w;
        
        float distanceToScene = distance(positionWS.xyz, scenePosWS.xyz);
        
        if(distanceToScene <= particleSize)
        {
            collision = true;
        }
    }
    
    return collision;
}

void getSceneNormalVectors(float3 positionWS, out float3 microNormal, out float3 macroNormal)
{
    float4 posWS = float4(positionWS, 1.0);
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float4 positionCS = mul(posWS - g_cameraPosition, g_viewProj);
#else
    float4 positionCS = mul(posWS, g_viewProj);
#endif
    
    positionCS /= positionCS.w;
    
    float2 uv = (positionCS.xy + 1) * 0.5;
    uv.y = 1.0 - uv.y;
    
    float4 normals = g_normal.SampleLevel(g_pointWrap, uv, 0);
    
    microNormal = unpackOctahedron(normals.rg);
    macroNormal = unpackOctahedron(normals.ba);
}
