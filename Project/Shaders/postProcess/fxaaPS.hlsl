/*-----------------------------------------------------------------------------
	Integrating NVidia FXAA code
-----------------------------------------------------------------------------*/

#define FXAA_PC 1 // PC platform
#define FXAA_HLSL_5 1 // Shader Model 5
#define FXAA_QUALITY__PRESET 39
    // 10 to 15 - default medium dither (10=fastest, 15=highest quality)
    // 20 to 29 - less dither, more expensive (20=fastest, 29=highest quality)
    // 39       - no dither, very expensive 

#include "../globals.hlsl"
#include "Fxaa3_11.hlsl"

/*---------------------------------------------------------------------------*/

Texture2D<float4> g_image : register(t20); // .rgb = LDR sRGB color (after gamma-correction), .a = color luma

cbuffer FxaaParams : register(b10)
{
    float4 g_imageSize; // .xy = image_size, .zw = 1.0 / image_size

    float g_qualitySubpix; //   FXAA_QUALITY__SUBPIX, range [0.0; 1.0], default 0.75
										//   Choose the amount of sub-pixel aliasing removal. This can effect sharpness.
										//   1.00 - upper limit (softer)
										//   0.75 - default amount of filtering
										//   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
										//   0.25 - almost off
										//   0.00 - completely off

    float g_qualityEdgeThreshold; //   FXAA_QUALITY__EDGE_THRESHOLD, range [0.063; 0.333], best quality 0.063
										//   The minimum amount of local contrast required to apply algorithm.
										//   0.333 - too little (faster)
										//   0.250 - low quality
										//   0.166 - default
										//   0.125 - high quality 
										//   0.063 - overkill (slower)

    float g_qualityEdgeThresholdMin; //   FXAA_QUALITY__EDGE_THRESHOLD_MIN, range [0.0; 0.0833], best quality 0.0312
										//   Trims the algorithm from processing darks.
										//   0.0833 - upper limit (default, the start of visible unfiltered edges)
										//   0.0625 - high quality (faster)
										//   0.0312 - visible limit (slower)
}

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

float4 main(vs_out input) : SV_TARGET
{
    FxaaTex TextureAndSampler;
    TextureAndSampler.tex = g_image;
    TextureAndSampler.smpl = g_bilinearClamp;
    TextureAndSampler.UVMinMax = float4(0, 0, 1, 1); // fullscreen uv

    return FxaaPixelShader(
		input.pos.xy * g_imageSize.zw, // map pixel coordinate to [0; 1] range
		0, // unused, for consoles
		TextureAndSampler,
		TextureAndSampler,
		TextureAndSampler,
		g_imageSize.zw,
		0, // unused, for consoles
		0, // unused, for consoles
		0, // unused, for consoles
		g_qualitySubpix,
		g_qualityEdgeThreshold,
		g_qualityEdgeThresholdMin,
		0, // unused, for consoles
		0, // unused, for consoles
		0, // unused, for consoles
		0 // unused, for consoles
	);
}