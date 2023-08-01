#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "../litShading.hlsl"

struct vs_out
{
    float4 position_clip : SV_POSITION;
    float2 texCoord : TEX;
    float time : TIME;
};

Texture2D g_noiseTex : TEXTURE : register(t23);

SurfacePoint getSurfacePoint(vs_out input);

float4 main(vs_out input) : SV_TARGET
{
    float timeThreshold = input.time;
    float noise = g_noiseTex.Sample(g_pointWrap, input.texCoord);

    float4 albedo = { 0.0, 0.0, 0.0, 1.0 };
    if (noise >= timeThreshold)
    {
        albedo.w = 0.0;
    }
	
    float alpha = albedo.w;
    float threshold = 0.2;
    float falloff = 0.05;
    float dx = ddx(input.texCoord);
    float dy = ddy(input.texCoord);

    float outAlpha = saturate(((alpha - threshold) / (abs(dx * alpha) + abs(dy * alpha))) / falloff);

    return float4(0.0, 0.0, 0.0, outAlpha);
}
