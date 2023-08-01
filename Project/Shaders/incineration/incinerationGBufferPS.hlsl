#include "../globals.hlsl"

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

//struct outGbuffer
//{
//    float4 albedo : SV_Target0;
//    float4 roughness_metalness : SV_Target1;
//    float4 normal : SV_Target2;
//    float4 emission : SV_Target3;
//    uint objectID : SV_Target4;
//};

cbuffer cbuffer_Material : register(b10)
{
    bool g_useDiffuseTexture;
    bool g_useNormalTexture;
    bool g_useRoughnessTexture;
    float g_roughness;

    bool g_useMetalnessTexture;
    float g_metalness;
};

cbuffer cbuffer_ParticleColor : register(b11)
{
    float3 g_particleColor;
}

Texture2D g_diffuseTex : TEXTURE : register(t20);
Texture2D g_normalTex : TEXTURE : register(t21);
Texture2D g_armTex : TEXTURE : register(t22);
Texture2D g_noiseTex : TEXTURE : register(t23);
Texture2D g_gbufferNormal : TEXTURE : register(t24);

static const float4 EMISSION_MAX_VALUE = float4(2.0, 0.45, 0.1, 1.0);

outGbuffer main(vs_out input)
{
    outGbuffer output;
    
    float4 albedo = g_diffuseTex.Sample(g_bilinearWrap, input.texCoord);
    float distanceToSphereCenter = distance(input.fragPos, input.spherePosition);
    float effectValue = (distanceToSphereCenter - input.sphereSmallRadius) / (input.sphereBigRadius - input.sphereSmallRadius);
    
    float alpha = 1.0;
    if(effectValue > 1.0)
    {
        output.emission = float4(0.0, 0.0, 0.0, 1.0);
    }
    if (effectValue >= 0.0 && effectValue <= 1.0)
    {
        alpha = g_noiseTex.Sample(g_pointWrap, input.texCoord);
        output.emission = float4((1.0 - effectValue).xxx, alpha) * float4(input.particleColor, 1.0);
    }
    else if (effectValue <= 0.0)
    {
        alpha = 0.0;
    }
    
    float threshold = 0.2;
    float falloff = 0.1;
    float dx = ddx(input.texCoord);
    float dy = ddy(input.texCoord);
    
    alpha = saturate(((alpha - threshold) / (abs(dx * alpha) + abs(dy * alpha))) / falloff);
    
    albedo.w = alpha;
    output.albedo = albedo;
    output.roughness_metalness = float4(g_armTex.Sample(g_bilinearWrap, input.texCoord).gb, 0.0, albedo.w);
    if (!g_useRoughnessTexture)
    {
        output.roughness_metalness.r = g_roughness;
    }
    if (g_useRoughnessOverwriting)
    {
        output.roughness_metalness.r = g_overwrittenRoughness;
    }
    if (!g_useMetalnessTexture)
    {
        output.roughness_metalness.g = g_metalness;
    }
    
    float3 geometryNormal = normalize(input.normal);
    
    float3 textureNormal;
    if (g_useNormalTexture)
    {
        float3 normalFromTexture = g_normalTex.Sample(g_trilinearWrap, input.texCoord);
        normalFromTexture = 2.0f * normalFromTexture - 1.0f;
        float3 normalTBN = mul(normalFromTexture, input.TBN);

        textureNormal = normalize(normalTBN);
    }
    else
    {
        textureNormal = geometryNormal;
    }
    
    float4 currentNormal = g_gbufferNormal.Load(int3(input.position_clip.xy, 0));
    
    float3 currentTextureNormal = unpackOctahedron(currentNormal.rg);
    textureNormal = normalize(lerp(currentTextureNormal, textureNormal, albedo.w));
    
    float3 currentGeometryNormal = unpackOctahedron(currentNormal.ba);
    geometryNormal = normalize(lerp(currentGeometryNormal, geometryNormal, albedo.w));
    
    output.normal = float4(packOctahedron(textureNormal), packOctahedron(geometryNormal));
    output.objectID = input.instanceNumber;
    
    return output;
}

