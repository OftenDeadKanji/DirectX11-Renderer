#include "../globals.hlsl"

struct vs_in
{
    float3 position_local : POS;
    float4 color_local : COL;
    float2 textureCoordinates : TEX;
    float3 normal : NORM;
    float3 tangent : TANG;
    float3 bitangent : BTANG;

    float4 decalToWorld0 : INS0;
    float4 decalToWorld1 : INS1;
    float4 decalToWorld2 : INS2;
    float4 decalToWorld3 : INS3;
    float4 worldToDecal0 : INSINV0;
    float4 worldToDecal1 : INSINV1;
    float4 worldToDecal2 : INSINV2;
    float4 worldToDecal3 : INSINV3;
    float3 color : CLR;
    uint objectID : OID;
};

struct vs_out
{
    float4 position_clip : SV_POSITION;
    row_major float4x4 decalToWorld : INS;
    row_major float4x4 worldToDecal : INSINV;
    float3 worldPos : POS;
    float3 color : CLR;
    uint objectID : OID;
};

vs_out main(vs_in input)
{
    vs_out output = (vs_out) 0;

    float4x4 modelToWorld = float4x4(input.decalToWorld0, input.decalToWorld1, input.decalToWorld2, input.decalToWorld3);

    float4 pos = float4(input.position_local, 1.0);
    float4 worldPos = mul(pos, modelToWorld);

    output.worldPos = worldPos.xyz;
    //output.position_clip = output.worldPos;
    output.position_clip = mul(worldPos, g_viewProj);

    output.decalToWorld = modelToWorld;
    output.worldToDecal = float4x4(input.worldToDecal0, input.worldToDecal1, input.worldToDecal2, input.worldToDecal3);
    
    output.color = input.color;
    output.objectID = input.objectID;
    
    return output;
}