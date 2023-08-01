#ifndef __GLOBALS_HLSL__
#define __GLOBALS_HLSL__

#ifndef MAX_POINT_LIGHTS
    #define MAX_POINT_LIGHTS 8
#endif

#ifndef SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    #define SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE 0
#endif

#ifndef MAX_GPU_PARTICLES
    #define MAX_GPU_PARTICLES 1000
#endif

#ifndef PI
    #define PI 3.14159265
#endif

#ifndef MAX_VOLUMETRIC_FOG_INSTANCES
    #define MAX_VOLUMETRIC_FOG_INSTANCES 8
#endif

cbuffer PerFrame : register(b0)
{
    float g_time; 
    float g_deltaTime;
    bool g_useDiffuseReflection;
    bool g_useSpecularReflection;
    bool g_useIBL;
    bool g_useRoughnessOverwriting;
    float g_overwrittenRoughness;
    uint g_specularIBLTextureMipLevels;
};

cbuffer PerView : register(b1)
{
    row_major float4x4 g_view;
    row_major float4x4 g_viewInv;
    row_major float4x4 g_proj;
    row_major float4x4 g_projInv;
    row_major float4x4 g_viewProj;
    row_major float4x4 g_viewProjInv;
    float3 g_cameraPosition;
    float g_zNear;
    float3 g_FrustumBLdir;
    float g_zFar;
    float3 g_FrustumTLdir;
    float3 g_FrustumBRdir;
};

SamplerState g_pointWrap : register(s0);
SamplerState g_bilinearWrap : register(s1);
SamplerState g_trilinearWrap : register(s2);
SamplerState g_anisotropicWrap : register(s3);
SamplerComparisonState  g_depthSampler : register(s4);
SamplerState g_bilinearClamp : register(s5);

struct DirectionalLight
{
    row_major float4x4 depthViewProj;
    row_major float4x4 depthViewProjInv;
    float3 energy;
    float solidAngle;
    float3 direction;
    float perceivedRadius;
    float perceivedDistance;
};

struct PointLight
{
    row_major float4x4 depthViewProj[6];
    row_major float4x4 depthViewProjInv[6];
    float3 energy;
    float radius;
    float3 position;
    float cameraZNear;
    float cameraZFar;
};

struct SpotLight
{
    row_major float4x4 depthViewProj;
    row_major float4x4 depthViewProjInv;
    row_major float4x4 projectionMatrix;
    float3 energy;
    float cosAngle;
    float3 position;
    float radius;
    float3 direction;
};
Texture2D<float4> g_spotLightTextue : TEXTURE: register(t0);

//IBL
TextureCube g_diffuseIBL : TEXTURE: register(t1);
TextureCube g_specularIBL : TEXTURE: register(t2);
Texture2D g_factorIBL : TEXTURE: register(t3);

Texture2D g_depthDirectionalLight : TEXTURE: register(t4);
TextureCubeArray g_depthPointLight : TEXTURE: register(t5);
Texture2D g_depthSpotLight : TEXTURE: register(t6);

cbuffer Lights : register(b2)
{
    float3 g_ambientLightEnergy;
    
    int g_pointLightsCount;

    PointLight g_pointLights[MAX_POINT_LIGHTS];
    
    DirectionalLight g_directionalLight;
    SpotLight g_spotLight;
};

struct outGbuffer
{
    float4 albedo : SV_Target0;
    float4 roughness_metalness : SV_Target1;
    float4 normal : SV_Target2;
    float4 emission : SV_Target3;
    uint objectID : SV_Target4;
};

struct outGbufferWithDepth
{
    float4 albedo : SV_Target0;
    float4 roughness_metalness : SV_Target1;
    float4 normal : SV_Target2;
    float4 emission : SV_Target3;
    uint objectID : SV_Target4;
    
    float depth : SV_Depth;
};

cbuffer GBufferInfo : register(b3)
{
    int2 g_gbufferTexturesSize;
    int2 g_gbufferPad;
};

struct VolumetricFogInstance
{
    row_major float4x4 fogToWorld;
    row_major float4x4 worldToFog;
};

cbuffer VolumetricFogInfo : register(b4)
{
    bool g_uniformFogEnabled;
    bool g_volumetricFogEnabled;
    float g_fogDensity;
    float g_phaseFunctionParameter;
    
    uint g_volumetricFogInstancesCount;
    //uint g_fogPad[3];
    
    VolumetricFogInstance g_volumetricFogInstances[MAX_VOLUMETRIC_FOG_INSTANCES];
};

float3 getViewDirection(float3 fragWorldPos);
void normalizeVector(float3 toNormalize, out float3 vNormalized, out float vLength);
float2 nonZeroSign(float2 v);
float2 packOctahedron(float3 v);
float3 unpackOctahedron(float2 oct);

float3 getViewDirection(float3 fragWorldPos)
{
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    return normalize(-fragWorldPos);
#else
    return normalize(g_cameraPosition - fragWorldPos);
#endif
}
void normalizeVector(float3 toNormalize, out float3 vNormalized, out float vLength)
{
    vLength = length(toNormalize);
    vNormalized = toNormalize / vLength;
}

float2 nonZeroSign(float2 v)
{
    return float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 packOctahedron(float3 v)
{
    float2 p = v.xy / (abs(v.x) + abs(v.y) + abs(v.z));
    return v.z <= 0.0 ? (float2(1.0, 1.0) - abs(p.yx)) * nonZeroSign(p) : p;
}

float3 unpackOctahedron(float2 oct)
{
    float3 v = float3(oct, 1.0 - abs(oct.x) - abs(oct.y));
    if (v.z < 0)
        v.xy = (float2(1.0, 1.0) - abs(v.yx)) * nonZeroSign(v.xy);
    return normalize(v);
}

#endif