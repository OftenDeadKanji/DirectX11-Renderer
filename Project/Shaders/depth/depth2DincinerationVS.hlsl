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
};

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 worldPos : POS;
    float2 texCoord : TEX;
    float3 spherePosition : SPHP;
    float sphereBigRadius : SPHBR;
    float sphereSmallRadius : SPHSR;
};

vs_out main(vs_in input)
{
    vs_out output = (vs_out) 0;

    float4x4 modelToWorld = float4x4(input.instanceMat0, input.instanceMat1, input.instanceMat2, input.instanceMat3);

    float4 pos = float4(input.position_local, 1.0);
    float4 worldPos = mul(pos, modelToWorld);
    
    output.worldPos = worldPos;
    output.position_clip = mul(worldPos, g_viewProj);
    
    output.texCoord = input.textureCoordinates;
    output.spherePosition = input.sphere.rgb;
    output.sphereBigRadius = input.sphere.w;
    output.sphereSmallRadius = input.sphereSmallRadius;

    return output;
}