#include "../globals.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    row_major float4x4 decalToWorld : INS;
    row_major float4x4 worldToDecal : INSINV;
    float3 worldPos : POS;
    float3 color : CLR;
    uint objectID : OID;
};

Texture2D g_decalTex : TEXTURE : register(t20);
Texture2D g_backgroundDepth: TEXTURE : register(t21);
Texture2D g_backgroundNormal : TEXTURE : register(t22);
Texture2D<uint> g_backgroundObjectID : TEXTURE : register(t23);

outGbufferWithDepth main(vs_out input)
{
    outGbufferWithDepth output;
    
    float2 xy = input.position_clip.xy / float2(g_gbufferTexturesSize.x, g_gbufferTexturesSize.y);
    uint objectID = g_backgroundObjectID.Load(int3(input.position_clip.xy, 0));
    
    if(objectID != input.objectID)
    {
        discard;
    }
    
    float depth = g_backgroundDepth.Sample(g_pointWrap, xy);
    output.depth = depth;
    
    float4 backgroundNormalTexVal = g_backgroundNormal.Sample(g_pointWrap, xy);
    float3 backgroundMicroNormal = unpackOctahedron(backgroundNormalTexVal.rg);
    float3 backgroundMacroNormal = unpackOctahedron(backgroundNormalTexVal.ba);
    
    float3 backgroundNormalInDecalSpace = mul(float4(backgroundMacroNormal, 0.0), input.worldToDecal).xyz;
    if(backgroundNormalInDecalSpace.z > 0)
    {
        discard;
    }
    
    xy = (xy - 0.5) * 2.0;
    xy.y *= -1.0;
    float4 posCS = float4(xy, depth, 1.0);
    
    float4 posWS = mul(posCS, g_viewProjInv);
    posWS /= posWS.w;
    
    float4 posDS = mul(posWS, input.worldToDecal);
    
    if (abs(posDS.x) > 1.0 || abs(posDS.y) > 1.0 || abs(posDS.z) > 1.0)
    {
        discard;
    }
    
    float2 uv = (posDS.xy + 1.0) * 0.5;
    uv.y = 1.0 - uv.y;
    
    float4 decalTexVal = g_decalTex.Sample(g_bilinearWrap, uv);
    
    float3 N = backgroundMacroNormal;
    float3 T = input.decalToWorld[0];
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T));
    
    float3x3 TBN = float3x3(T, B, N);
    float3 decalNormal = decalTexVal.rgb;
    decalNormal = mul(decalNormal, TBN);
    decalNormal = normalize(lerp(backgroundMicroNormal, decalNormal, decalTexVal.a));
    
    output.normal = float4(packOctahedron(decalNormal), backgroundNormalTexVal.ba);
    output.albedo = float4(input.color, decalTexVal.a);
    
    output.roughness_metalness = float4(0.1, 0.0, 0.0, decalTexVal.a);
    
    return output;
}