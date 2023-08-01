#include "../globals.hlsl"

struct vs_out
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEX;
};

Texture2D<float4> g_hdrRender : TEXTURE: register(t20);

cbuffer Params : register(b10)
{
	float g_gamma;
	float g_EV100;
};

float3 acesHdr2Ldr(float3 hdr)
{
	float3x3 m1 = float3x3(
		float3(0.59719f, 0.07600f, 0.02840f),
		float3(0.35458f, 0.90834f, 0.13383f),
		float3(0.04823f, 0.01566f, 0.83777f)
		);
	float3x3 m2 = float3x3(
		float3(1.60475f, -0.10208, -0.00327f),
		float3(-0.53108f, 1.10813, -0.07276f),
		float3(-0.07367f, -0.00605, 1.07602f)
		);

	float3 v = mul(hdr, m1);
	float3 a = v * (v + float3(0.0245786f, 0.0245786f, 0.0245786f)) - float3(0.000090537f, 0.000090537f, 0.000090537f);
	float3 b = v * (float3(0.983729f, 0.983729f, 0.983729f) * v + float3(0.4329510f, 0.4329510f, 0.4329510f)) + float3(0.238081f, 0.238081f, 0.238081f);
	float3 ldr = saturate(mul(a / b, m2));

	return ldr;
}

float3 adjustExposure(float3 color, float EV100)
{
	float LMax = (78.0f / (0.65f * 100.0f)) * pow(2.0f, EV100);
	return color * (1.0f / LMax);
}

float3 correctGamma(float3 color, float gamma)
{
	return pow(color, 1.0f / gamma);
}

void writeLuma(inout float4 color)
{
    float luma = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    color.a = luma;
}

float4 main(vs_out input) : SV_TARGET
{
    float3 hdrColor = g_hdrRender.Sample(g_pointWrap, input.texCoord);
    
    float3 ldrColor = adjustExposure(hdrColor, g_EV100);
    ldrColor = acesHdr2Ldr(ldrColor);
		
    float4 outColor = float4(correctGamma(ldrColor, g_gamma), 1.0);

    writeLuma(outColor);
	
	return outColor;
}