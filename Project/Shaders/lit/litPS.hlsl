#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "../litShading.hlsl"

struct vs_out
{
	float4 position_clip : SV_POSITION;
	float3x3 TBN : TBN;
	float3 fragPos : POS;
	float3 color : COL;
	float3 normal : NORM;
	float2 texCoord : TEX;
    uint instanceNumber : INS_NUMBER;
};

cbuffer cbuffer_Material : register(b10)
{
	bool g_useDiffuseTexture;
	bool g_useNormalTexture;
	bool g_useRoughnessTexture;
	float g_roughness;

	bool g_useMetalnessTexture;
	float g_metalness;
};

Texture2D g_diffuseTex : TEXTURE: register(t20);
Texture2D g_normalTex : TEXTURE: register(t21);
Texture2D g_armTex : TEXTURE: register(t22);

SurfacePoint getSurfacePoint(vs_out input);

float4 main(vs_out input) : SV_TARGET
{
	SurfacePoint surfacePoint = getSurfacePoint(input);
	
	float3 viewDirection = getViewDirection(input.fragPos);

	float3 directLight = float3(0.0, 0.0, 0.0);

	{
		float3 lightVal = lighting_directinalLight(g_directionalLight.energy, g_directionalLight.direction, g_directionalLight.solidAngle, g_directionalLight.perceivedRadius, g_directionalLight.perceivedDistance, surfacePoint, viewDirection);
		float visibility = calculateVisibilityForDirectionalLight(surfacePoint, g_directionalLight.direction, g_directionalLight.depthViewProj, g_directionalLight.depthViewProjInv);
		
		directLight += lightVal * visibility;
	}
	
	for (int i = 0; i < g_pointLightsCount; i++)
	{
		float3 lightVal = lighting_pointLight(g_pointLights[i].energy, g_pointLights[i].position, g_pointLights[i].radius, surfacePoint, viewDirection);
        float visibility = calculateVisibilityForPointLight(surfacePoint, g_pointLights[i].position, g_pointLights[i].depthViewProj, g_pointLights[i].depthViewProjInv, i, g_pointLights[i].cameraZNear, g_pointLights[i].cameraZFar);
		
		directLight += lightVal * visibility;
	}

	{
		float4 posBySpotLight = mul(float4(surfacePoint.worldPosition, 1.0), g_spotLight.projectionMatrix);
		float2 uv = posBySpotLight.xy / posBySpotLight.w;
		uv.y *= -1.0;

		float3 lightVal = lighting_spotLight(g_spotLight.energy, g_spotLight.position, g_spotLight.direction, g_spotLight.cosAngle, g_spotLight.radius, surfacePoint, viewDirection, uv);
		float visibility = calculateVisibilityForSpotLight(surfacePoint, g_spotLight.position, g_spotLight.depthViewProj, g_spotLight.depthViewProjInv);
		
		directLight += lightVal * visibility;
	}
	
	float3 result = directLight;
	if (g_useIBL)
	{
		if (g_useDiffuseReflection)
		{
			float3 diffuseIBL = g_diffuseIBL.Sample(g_bilinearWrap, surfacePoint.microNormal) * surfacePoint.material.color.rgb * (1 - surfacePoint.material.metalness);
			result += diffuseIBL;
		}

		if (g_useSpecularReflection)
		{
			float NdotV = dot(surfacePoint.microNormal, viewDirection);
			float3 factorIBL = g_factorIBL.Sample(g_bilinearWrap, float2(surfacePoint.material.roughness, max(0.0, NdotV)));
			
			float3 r = reflect(-viewDirection, surfacePoint.microNormal);

            uint mipLevel = g_specularIBLTextureMipLevels * surfacePoint.material.roughness;
			float3 specularIBL = g_specularIBL.SampleLevel(g_trilinearWrap, r, mipLevel) * (surfacePoint.material.F0 * factorIBL.r + factorIBL.g);

			result += specularIBL;
		}
	}

	return float4(result, surfacePoint.material.color.w);
}
SurfacePoint getSurfacePoint(vs_out input)
{
	SurfacePoint surfacePoint;
	surfacePoint.worldPosition = input.fragPos;

	surfacePoint.material.color = g_useDiffuseTexture ? g_diffuseTex.Sample(g_trilinearWrap, input.texCoord) : float4(input.color, 1.0);

	surfacePoint.macroNormal = normalize(input.normal);

	if (g_useNormalTexture)
	{
		float3 normalFromTexture = g_normalTex.Sample(g_trilinearWrap, input.texCoord);
		normalFromTexture = 2.0f * normalFromTexture - 1.0f;
		float3 normalTBN = mul(normalFromTexture, input.TBN);

		surfacePoint.microNormal = normalize(normalTBN);
	}
	else
	{
		surfacePoint.microNormal = surfacePoint.macroNormal;
	}

	surfacePoint.material.metalness = g_useMetalnessTexture ? g_armTex.Sample(g_trilinearWrap, input.texCoord).b : g_metalness;

	if (g_useRoughnessOverwriting)
	{
		surfacePoint.material.roughness = g_overwrittenRoughness;
	}
	else
	{
		surfacePoint.material.roughness = g_useRoughnessTexture ? g_armTex.Sample(g_trilinearWrap, input.texCoord).g : g_roughness;
	}
	surfacePoint.material.roughness2 = surfacePoint.material.roughness * surfacePoint.material.roughness;
	surfacePoint.material.roughness4 = surfacePoint.material.roughness2 * surfacePoint.material.roughness2;

	surfacePoint.material.F0 = lerp(0.04, surfacePoint.material.color, surfacePoint.material.metalness);

	return surfacePoint;
}
