#ifndef __BRDF_HLSL__
#define __BRDF_HLSL__

#ifndef PI
	#define PI 3.14159265
#endif

float3 brdf_LambertianDiffuse(float3 color, float metalness, float3 F0, float NoL, float solidAngle);
float3 brdf_CookTorranceSpecular(float roughness4, float3 F0, float NoH, float NoV, float NoL, float HoL, float solidAngle);
float3 fresnel(float NdotL, float3 F0);
float smith(float rough2, float NoV, float NoL);
float ggx(float rough2, float NoH);

float3 brdf_LambertianDiffuse(float3 color, float metalness, float3 F0, float NoL, float solidAngle)
{
	float3 Fn = fresnel(NoL, F0);
	return ((solidAngle * color * (1.0 - metalness)) / PI) * (1.0 - Fn * NoL);
}

float3 brdf_CookTorranceSpecular(float roughness4, float3 F0, float NoH, float NoV, float NoL, float HoL, float solidAngle)
{
	float D = ggx(roughness4, NoH);
	float G = smith(roughness4, NoV, NoL);
	float3 Fh = fresnel(HoL, F0);

	return min(1.0, (D * solidAngle) / (4.0 * max(0.0001, NoV))) * G * Fh;
}

float3 fresnel(float NdotL, float3 F0)
{
	return F0 + (1 - F0) * pow(1 - NdotL, 5);
}

float smith(float rough2, float NoV, float NoL)
{
	NoV *= NoV;
	NoL *= NoL;
	return 2.0 / (sqrt(1 + rough2 * (1 - NoV) / NoV) + sqrt(1 + rough2 * (1 - NoL) / NoL));
}

float ggx(float rough2, float NoH)
{
	float denom = NoH * NoH * (rough2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return rough2 / denom;
}

#endif