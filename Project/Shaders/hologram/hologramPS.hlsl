#include "../globals.hlsl"
#include "distortion.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float3 frag_pos : POS0;
    float3 frag_pos2 : POS1;
    float3 normal : NORM;
    nointerpolation float3 color : COL;
    uint instanceNumber : INS_NUMBER;
};

float4 main(vs_out input) : SV_TARGET
{
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float3 toCamera = normalize(input.frag_pos2);
#else
    float3 toCamera = normalize(g_cameraPosition - input.frag_pos2);
#endif

    return float4(colorDistortion(input.frag_pos, normalize(input.normal), input.color, toCamera), 1.0);
}
