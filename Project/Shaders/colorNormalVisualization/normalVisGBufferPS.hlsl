#include "../globals.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 normal : NORM;
    uint instanceNumber : INS_NUMBER;
};

outGbuffer main(vs_out input)
{
    outGbuffer output;
    
    output.emission = float4((normalize(input.normal) + 1.0) * 0.5, 1.0);
    output.objectID = input.instanceNumber;
    
    return output;
}