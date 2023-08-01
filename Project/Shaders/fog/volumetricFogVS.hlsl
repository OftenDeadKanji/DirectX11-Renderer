#include "../globals.hlsl"

struct vs_in
{
    uint vertexID : SV_VertexID;
    uint instanceID : SV_InstanceID;
};

struct vs_out
{
    float4 pos_clip : SV_Position;
    float3 modelPos : MODPOS;
    float3 worldPos : WPOS;
    float4x4 modelToWorld : MOD;
    float4x4 worldToModel : MODINV;
};

static const float3 vertices[] =
{
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f}
};

vs_out main(vs_in input)
{
    vs_out output;
    
    float4x4 localToWorld = g_volumetricFogInstances[input.instanceID].fogToWorld;
    float4x4 worldToLocal = g_volumetricFogInstances[input.instanceID].worldToFog;
    
    float4 posMS = float4(vertices[input.vertexID], 1.0);
    float4 posWS = mul(posMS, localToWorld);
    float4 posCS = mul(posWS, g_viewProj);
    
    output.pos_clip = posCS;
    output.modelPos = posMS.xyz;
    output.worldPos = posWS.xyz;
    output.modelToWorld = localToWorld;
    output.worldToModel = worldToLocal;
    return output;
}