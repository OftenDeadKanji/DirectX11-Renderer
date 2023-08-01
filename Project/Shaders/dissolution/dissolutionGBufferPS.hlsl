#include "../globals.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3x3 TBN : TBN;
    float3 fragPos : POS;
    float3 color : COL;
    float3 normal : NORM;
    float2 texCoord : TEX;
    float time : TIME;
    uint instanceNumber : INS_NUMBER;
};

cbuffer cbuffer_Material : register(b10)
{
    bool g_useDiffuseTexture;
    bool g_useNormalTexture;
    bool g_useRoughnessTexture;
    float g_roughness;

    bool g_useMetalnessTexture;
    float g_metalness;
};

Texture2D g_diffuseTex : TEXTURE : register(t20);
Texture2D g_normalTex : TEXTURE : register(t21);
Texture2D g_armTex : TEXTURE : register(t22);
Texture2D g_noiseTex : TEXTURE : register(t23);
Texture2D g_gbufferNormal : TEXTURE : register(t24);

outGbuffer main(vs_out input)
{
    outGbuffer output;
    
    float4 albedo = g_diffuseTex.Sample(g_bilinearWrap, input.texCoord);
    float timeThreshold = input.time;
    float noise = g_noiseTex.Sample(g_pointWrap, input.texCoord);

    if (noise >= timeThreshold)
    {
        albedo.w = 0.0;
    }

    if (noise >= timeThreshold - 0.05)
    {
        albedo.rgb = float3(1.0, 1.0, 1.0);
    }
    
    float alpha = albedo.w;
    float threshold = 0.2;
    float falloff = 0.05;
    float dx = ddx(input.texCoord);
    float dy = ddy(input.texCoord);

    albedo.w = saturate(((alpha - threshold) / (abs(dx * alpha) + abs(dy * alpha))) / falloff);

    output.albedo = albedo;
    output.roughness_metalness = float4(g_armTex.Sample(g_bilinearWrap, input.texCoord).gb, 0.0, albedo.w);
    if(!g_useRoughnessTexture)
    {
        output.roughness_metalness.r = g_roughness;
    }
    if(g_useRoughnessOverwriting)
    {
        output.roughness_metalness.r = g_overwrittenRoughness;
    }
    if(!g_useMetalnessTexture)
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
    output.emission = float4(0.0, 0.0, 0.0, 1.0);
    output.objectID = input.instanceNumber;
    
    return output;
}

