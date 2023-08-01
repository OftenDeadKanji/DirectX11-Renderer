#include "../globals.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 fragPos : POS;
    float3 normal : NORM;
    nointerpolation float3 color : COL;
    uint instanceNumber : INS_NUMBER;
};

outGbuffer main(vs_out input)
{
    outGbuffer output;
    
    float3 normal = normalize(input.normal);
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float3 cameraDir = normalize(-input.fragPos);
#else
    float3 cameraDir = normalize(g_cameraPosition - input.fragPos);
#endif

    float3 normedEmission = input.color / max(input.color.x, max(input.color.y, max(input.color.z, 1.0)));

    float NoV = dot(cameraDir, normal);
    
    //output.emission = float4(lerp(normedEmission * 0.33, input.color, pow(max(0.0, NoV), 8)), 1.0);
    output.emission = float4(input.color, 1.0);
    output.objectID = input.instanceNumber;
    
    return output;
}
