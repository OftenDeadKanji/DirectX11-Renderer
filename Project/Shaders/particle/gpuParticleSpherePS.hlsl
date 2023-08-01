#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "../litShading.hlsl"
#include "../fog/fog.hlsl"

struct vs_out
{
    float4 positionCS       : SV_Position;
    float3 particleCenter   : CTR;
    float4 particleColor    : COL;
};

Texture2D g_albedo                  : TEXTURE : register(t20);
Texture2D g_roughness_metalness     : TEXTURE : register(t21);
Texture2D g_normal                  : TEXTURE : register(t22);
Texture2D g_sceneDepthTexture       : TEXTURE : register(t23);

cbuffer FogParams : register(b10)
{
    float absorptionCoefficient;
    float scatteringCoefficient;
    float phaseFunctionParameter;
};

float4 main(vs_out input) : SV_TARGET
{
    float3 surfaceColor = g_albedo.Load(int3(input.positionCS.xy, 0)).rgb;
    float2 roughness_metalness = g_roughness_metalness.Load(int3(input.positionCS.xy, 0)).rg;
    float3 F0 = lerp(0.04, surfaceColor, roughness_metalness.g);
    
    float4 texAndGeomNormals = g_normal.Load(int3(input.positionCS.xy, 0));
    
    float3 microNormal = unpackOctahedron(texAndGeomNormals.rg);
    float3 macroNormal = unpackOctahedron(texAndGeomNormals.ba);
    
    float sceneDepth = g_sceneDepthTexture.Load(int3(input.positionCS.xy, 0));
    
    float2 texSize;
    g_sceneDepthTexture.GetDimensions(texSize.x, texSize.y);
    
    float4 surfacePosCS = float4((input.positionCS.xy / texSize - 0.5) * 2, sceneDepth, 1.0);
    surfacePosCS.y *= -1;
    float4 surfacePosWS = mul(surfacePosCS, g_viewProjInv);
    surfacePosWS /= surfacePosWS.w;
    
    float3 fragToLight = input.particleCenter - surfacePosWS.xyz;
    float3 fragToLightNormalized;
    float fragToLightLength;
    normalizeVector(fragToLight, fragToLightNormalized, fragToLightLength);
    
    float lightRadius = 0.2;
    
    float3 l = fragToLightNormalized;
    float3 v = getViewDirection(surfacePosWS.xyz);
	float3 r = reflect(-v, microNormal);

    float solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - min(1.0, pow(lightRadius / fragToLightLength, 2.0))));

	float hMacro = dot(fragToLight, macroNormal);
	float hMicro = dot(fragToLight, microNormal);
	float falloffMacro = min(1, (hMacro + lightRadius) / (2 * lightRadius));
	float falloffMicro = min(1, (hMicro + lightRadius) / (2 * lightRadius));

	float falloff = max(0.0, falloffMacro) * max(0.0, falloffMicro);
	solidAngle *= falloff;

	bool intersects;
	float cosAngularHalfSize = sqrt(1 - pow(lightRadius / fragToLightLength, 2));
	l = approximateClosestSphereDir(intersects, r, cosAngularHalfSize, fragToLight, l, fragToLightLength, lightRadius);

	float NoD = dot(microNormal, l);
	clampDirToHorizon(l, NoD, microNormal, 0.0);

	float NoL = dot(microNormal, l);
	if (NoL < 0.0001)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

    float3 diffuse = g_useDiffuseReflection ? brdf_LambertianDiffuse(surfaceColor, roughness_metalness.g, F0, NoL, solidAngle) : float3(0.0, 0.0, 0.0);
    float3 color = input.particleColor.rgb * diffuse;
    
    float3 dirLightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
    
    float d = g_zNear * g_zFar / (g_zFar + sceneDepth * (g_zNear - g_zFar));
    float cosTheta = dot(fragToLight, v);
    
    float3 colorAfterFog = calculateFog(color, dirLightEnergy, d, g_fogDensity, g_phaseFunctionParameter, cosTheta);
    
    return float4(colorAfterFog, input.particleColor.a);
}