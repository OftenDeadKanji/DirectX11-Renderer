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

    float3 instanceColor : INSCOL;
    uint instanceNumber : INS_NUMBER;
};

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 fragPos : POS;
    float3 normal : NORM;
    nointerpolation float3 color : COL;
    uint instanceNumber : INS_NUMBER;
};

vs_out main(vs_in input)
{
    vs_out output = (vs_out)0;

    float4x4 modelToWorld = float4x4(input.instanceMat0, input.instanceMat1, input.instanceMat2, input.instanceMat3);

    float4 pos = float4(input.position_local, 1.0);
    float4 worldPos = mul(pos, modelToWorld);

    output.fragPos = worldPos.xyz;

    output.position_clip = mul(worldPos, g_viewProj);

    float3 axisX = normalize(input.instanceMat0.xyz);
    float3 axisY = normalize(input.instanceMat1.xyz);
    float3 axisZ = normalize(input.instanceMat2.xyz);

    float3 normal = input.normal.x * axisX + input.normal.y * axisY + input.normal.z * axisZ;
    
    output.normal = normal;

    output.color = input.instanceColor;
    output.instanceNumber = input.instanceNumber;
    
    return output;
}
