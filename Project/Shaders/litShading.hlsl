#ifndef __LIT_SHADING_HLSL__
#define __LIT_SHADING_HLSL__

struct Material
{
	float4 color;
	float roughness;
	float roughness2;
	float roughness4;
	float metalness;
	float3 F0;
};
struct SurfacePoint
{
	float3 worldPosition;
	float3 macroNormal;
	float3 microNormal;

	Material material;
};
struct LightingContext
{
	float3 l; // light direction
	float3 v; // view direction
	float3 h; // half vector

	float solidAngle;

	float NoL;
	float NoV;
	float HoL;
	float NoH;
};

float3 blinnPhong(float3 albedo, float3 lightDirection, float3 normal, float3 viewDirection);
float3 lighting_directinalLight(float3 lightEnergy, float3 lightDirection, float solidAngle, float perceivedRadius, float perceivedDistance, SurfacePoint surfacePoint, float3 viewDirection);
float3 lighting_pointLight(float3 lightEnergy, float3 lightPosition, float lightRadius, SurfacePoint surfacePoint, float3 viewDirection);
float3 lighting_spotLight(float3 lightEnergy, float3 lightPosition, float3 lightDirection, float lightCosAngle, float lightRadius, SurfacePoint surfacePoint, float3 viewDirection, float2 spotLightUV);

float3 approximateClosestSphereDir(out bool intersects, float3 reflectionDir, float sphereCos, float3 sphereRelPos, float3 sphereDir, float sphereDist, float sphereRadius);
void clampDirToHorizon(inout float3 dir, inout float NoD, float3 normal, float minNoD);

void basisFromDir(out float3 right, out float3 top, in float3 dir);
float3x3 basisFromDir(float3 dir);

float calculateTexelSizeForDirectinalLightShadow(float textureSize, float4x4 depthViewProjInv);
float calculateVisibilityForDirectionalLight(SurfacePoint surfacePoint, float3 lightDirection, float4x4 lightDepthViewProj, float4x4 lightDepthViewProjInv);

float calculateTexelSizeForPointLightShadow(float textureSize, float depth, float4x4 depthViewProjInv, float lightCameraZNear, float lightCameraZFar);
float calculateVisibilityForPointLight(SurfacePoint surfacePoint, float3 lightPosition, float4x4 lightDepthViewProj[6], float4x4 lightDepthViewProjInv[6], int lightIndex, float lightCameraZNear, float lightCameraZFar);

float calculateTexelSizeForSpotLightShadow(float textureSize, float depth, float4x4 depthViewProjInv);
float calculateVisibilityForSpotLight(SurfacePoint surfacePoint, float3 lightPosition, float4x4 lightDepthViewProj, float4x4 lightDepthViewProjInv);


float3 blinnPhong(float3 albedo, float3 lightDirection, float3 normal, float3 viewDirection)
{
	float3 halfVector = normalize(lightDirection + viewDirection);

	float diffuse = max(dot(lightDirection, normal), 0.0001);

	float shininess = 32.0;
	float specular = pow(max(dot(halfVector, normal), 0.0001), shininess);

	return diffuse * albedo + specular;
}
float3 lighting_directinalLight(float3 lightEnergy, float3 lightDirection, float solidAngle, float perceivedRadius, float perceivedDistance, SurfacePoint surfacePoint, float3 viewDirection)
{
	LightingContext context;

	context.l = normalize(-lightDirection);
	context.v = viewDirection;
	context.h = normalize(context.l + context.v);

	context.NoL = dot(surfacePoint.microNormal, context.l);
	if (context.NoL < 0.0001)
	{
		return float3(0.0, 0.0, 0.0);
	}
	context.NoV = max(0.0001, dot(surfacePoint.microNormal, context.v));
	context.HoL = dot(context.h, context.l);
	context.NoH = max(0.0001, dot(surfacePoint.microNormal, context.h));

	float3 I = lightEnergy;

	float3 fragToLight = context.l * perceivedDistance;

	float hMacro = dot(fragToLight, surfacePoint.macroNormal);
	float hMicro = dot(fragToLight, surfacePoint.microNormal);
	float falloffMacro = min(1, (hMacro + perceivedRadius) / (2 * perceivedRadius));
	float falloffMicro = min(1, (hMicro + perceivedRadius) / (2 * perceivedRadius));

	float falloff = max(0.0, falloffMacro) * max(0.0, falloffMicro);
	context.solidAngle = solidAngle * falloff;

	float3 brdf = float3(0.0, 0.0, 0.0);
	if (g_useDiffuseReflection)
	{
		float3 diffuse = brdf_LambertianDiffuse(surfacePoint.material.color, surfacePoint.material.metalness, surfacePoint.material.F0, context.NoL, context.solidAngle);
		brdf += diffuse;
	}
	if (g_useSpecularReflection)
	{
		float3 specular = brdf_CookTorranceSpecular(surfacePoint.material.roughness4, surfacePoint.material.F0, context.NoH, context.NoV, context.NoL, context.HoL, context.solidAngle);
		brdf += specular;
	}
	float3 L = I * brdf;

	return L;
}
float3 lighting_pointLight(float3 lightEnergy, float3 lightPosition, float lightRadius, SurfacePoint surfacePoint, float3 viewDirection)
{
	LightingContext context;

	float3 fragToLight = lightPosition - surfacePoint.worldPosition;
	float3 fragToLightNormalized;
    float fragToLightLength;
	normalizeVector(fragToLight, fragToLightNormalized, fragToLightLength);

	context.l = fragToLightNormalized;
	context.v = viewDirection;
	context.h = normalize(context.l + context.v);
	float3 r = reflect(-context.v, surfacePoint.microNormal);

	context.solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - min(1.0, pow(lightRadius / fragToLightLength, 2.0))));

	float hMacro = dot(fragToLight, surfacePoint.macroNormal);
	float hMicro = dot(fragToLight, surfacePoint.microNormal);
	float falloffMacro = min(1, (hMacro + lightRadius) / (2 * lightRadius));
	float falloffMicro = min(1, (hMicro + lightRadius) / (2 * lightRadius));

	float falloff = max(0.0, falloffMacro) * max(0.0, falloffMicro);
	context.solidAngle *= falloff;

	bool intersects;
	float cosAngularHalfSize = sqrt(1 - pow(lightRadius / fragToLightLength, 2));
	context.l = approximateClosestSphereDir(intersects, r, cosAngularHalfSize, fragToLight, context.l, fragToLightLength, lightRadius);

	float NoD = dot(surfacePoint.microNormal, context.l);
	clampDirToHorizon(context.l, NoD, surfacePoint.microNormal, 0.0);

	context.h = normalize(context.l + context.v);
	
    context.NoL = dot(surfacePoint.microNormal, context.l);
	if (context.NoL < 0.0001)
	{
		return float3(0.0, 0.0, 0.0);
	}
	context.NoV = max(0.0001, dot(surfacePoint.microNormal, context.v));
	context.HoL = dot(context.h, context.l);
	context.NoH = max(0.0001, dot(surfacePoint.microNormal, context.h));

	float3 I = lightEnergy;

	float3 brdf = float3(0.0, 0.0, 0.0);
	if (g_useDiffuseReflection)
	{
		float3 diffuse = brdf_LambertianDiffuse(surfacePoint.material.color, surfacePoint.material.metalness, surfacePoint.material.F0, context.NoL, context.solidAngle);
		brdf += diffuse;
	}
	if (g_useSpecularReflection)
	{
		float3 specular = brdf_CookTorranceSpecular(surfacePoint.material.roughness4, surfacePoint.material.F0, context.NoH, context.NoV, context.NoL, context.HoL, context.solidAngle);
		brdf += specular;
	}
	float3 L = I * brdf;

	return L;
}
float3 lighting_spotLight(float3 lightEnergy, float3 lightPosition, float3 lightDirection, float lightCosAngle, float lightRadius, SurfacePoint surfacePoint, float3 viewDirection, float2 spotLightUV)
{
	LightingContext context;

	float3 fragToLight = lightPosition - surfacePoint.worldPosition;
    float3 fragToLightNormalized;
	float fragToLightLength;
	
	normalizeVector(fragToLight, fragToLightNormalized, fragToLightLength);

	float cosToFrag = dot(-fragToLightNormalized, lightDirection);

	float visible = cosToFrag - lightCosAngle >= 0.0 ? 1.0 : 0.0;

	context.l = fragToLightNormalized;
	context.v = viewDirection;
	context.h = normalize(context.l + context.v);
	float3 r = reflect(-context.v, surfacePoint.microNormal);

	context.solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - min(1.0, pow(lightRadius / fragToLightLength, 2.0))));

	float hMacro = dot(fragToLight, surfacePoint.macroNormal);
	float hMicro = dot(fragToLight, surfacePoint.microNormal);
	float falloffMacro = min(1, (hMacro + lightRadius) / (2 * lightRadius));
	float falloffMicro = min(1, (hMicro + lightRadius) / (2 * lightRadius));

	float falloff = max(0.0, falloffMacro) * max(0.0, falloffMicro);
	context.solidAngle *= falloff;

	bool intersects;
	float cosAngularHalfSize = sqrt(1 - pow(lightRadius / fragToLightLength, 2));
	context.l = approximateClosestSphereDir(intersects, r, cosAngularHalfSize, fragToLight, context.l, fragToLightLength, lightRadius);

	float NoD = dot(surfacePoint.microNormal, context.l);
	clampDirToHorizon(context.l, NoD, surfacePoint.microNormal, 0.0);

	context.h = normalize(context.l + context.v);
	
	context.NoL = dot(surfacePoint.microNormal, context.l);
	if (context.NoL < 0.0001)
	{
		return float3(0.0, 0.0, 0.0);
	}
	context.NoV = max(0.0001, dot(surfacePoint.microNormal, context.v));
	context.HoL = dot(context.h, context.l);
	context.NoH = max(0.0001, dot(surfacePoint.microNormal, context.h));

	float3 I = lightEnergy;

	float3 diffuse = brdf_LambertianDiffuse(surfacePoint.material.color, surfacePoint.material.metalness, surfacePoint.material.F0, context.NoL, context.solidAngle);
	float3 specular = brdf_CookTorranceSpecular(surfacePoint.material.roughness4, surfacePoint.material.F0, context.NoH, context.NoV, context.NoL, context.HoL, context.solidAngle);

	float3 L = I * (diffuse + specular);

	float4 spotLightTexColor = g_spotLightTextue.Sample(g_bilinearWrap, spotLightUV);

	return L * (1.0 - spotLightTexColor.w) * visible;
}

float3 approximateClosestSphereDir(out bool intersects, float3 reflectionDir, float sphereCos, float3 sphereRelPos, float3 sphereDir, float sphereDist, float sphereRadius)
{
	float RoS = dot(reflectionDir, sphereDir);

	intersects = RoS >= sphereCos;
	if (intersects) return reflectionDir;
	if (RoS < 0.0) return sphereDir;

	float3 closestPointDir = normalize(reflectionDir * sphereDist * RoS - sphereRelPos);
	return normalize(sphereRelPos + sphereRadius * closestPointDir);
}
void clampDirToHorizon(inout float3 dir, inout float NoD, float3 normal, float minNoD)
{
	if (NoD < minNoD)
	{
		dir = normalize(dir + (minNoD - NoD) * normal);
		NoD = minNoD;
	}
}

void basisFromDir(out float3 right, out float3 top, in float3 dir)
{
	float k = 1.0 / max(1.0 + dir.z, 0.00001);
	float a = dir.y * k;
	float b = dir.y * a;
	float c = -dir.x * a;
	right = float3(dir.z + b, c, -dir.x);
	top = float3(c, 1.0 - b, -dir.y);
}
float3x3 basisFromDir(float3 dir)
{
	float3x3 rotation;
	rotation[2] = dir;
	basisFromDir(rotation[0], rotation[1], dir);
	return rotation;
}

float calculateTexelSizeForDirectinalLightShadow(float textureSize, float4x4 depthViewProjInv)
{
	float4  firstTexelCenterCS = { (1.0 / textureSize) - 1.0,	(1.0 / textureSize) - 1.0, 0.0, 1.0 };
	float4 secondTexelCenterCS = { (3.0 / textureSize) - 1.0,	(1.0 / textureSize) - 1.0, 0.0, 1.0 };

	float4  firstTexelCenterWS = mul(firstTexelCenterCS, depthViewProjInv);
	firstTexelCenterWS /= firstTexelCenterWS.w;

	float4 secondTexelCenterWS = mul(secondTexelCenterCS, depthViewProjInv);
	secondTexelCenterWS /= secondTexelCenterWS.w;

	return abs(firstTexelCenterWS.x - secondTexelCenterWS.x);
}
float calculateVisibilityForDirectionalLight(SurfacePoint surfacePoint, float3 lightDirection, float4x4 lightDepthViewProj, float4x4 lightDepthViewProjInv)
{
	float2 textureSize;
	g_depthDirectionalLight.GetDimensions(textureSize.x, textureSize.y);

	float texelSize = calculateTexelSizeForDirectinalLightShadow(textureSize.x, lightDepthViewProjInv);
	float3 offset = texelSize * sqrt(2.0) / 2.0 * (surfacePoint.macroNormal - 0.9 * -lightDirection * dot(surfacePoint.macroNormal, -lightDirection));

	float4 worldPos4 = float4(surfacePoint.worldPosition + offset, 1.0);

	float4 posInLightClipSpace = mul(worldPos4, lightDepthViewProj);
	posInLightClipSpace /= posInLightClipSpace.w;

	float2 uvShadow = (posInLightClipSpace * 0.5 + 0.5).xy;
	uvShadow.y = 1.0 - uvShadow.y;

	float2 uvOffset[4] = {
		{-0.5 / textureSize.x,  0.0},
		{ 0.5 / textureSize.x,  0.0},
		{ 0.0, -0.5 / textureSize.x},
		{ 0.0,  0.5 / textureSize.x}
	};

	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		visibility += g_depthDirectionalLight.SampleCmp(g_depthSampler, uvShadow + uvOffset[i], posInLightClipSpace.z);
	}

	visibility *= 0.25;
	visibility = smoothstep(0.33, 1.0, visibility);

	return visibility;
}

float calculateTexelSizeForPointLightShadow(float textureSize, float depth, float4x4 depthViewProjInv, float lightCameraZNear, float lightCameraZFar)
{
    float texelSizeForNearPlane = (2 * lightCameraZNear) * 1.0 / textureSize;
	
	//linearized depth
    float depthLin = (lightCameraZNear * lightCameraZFar) / (lightCameraZFar - (1.0 - depth) * (lightCameraZFar - lightCameraZNear));
	
    return (depthLin / lightCameraZNear) * texelSizeForNearPlane;
}
float calculateVisibilityForPointLight(SurfacePoint surfacePoint, float3 lightPosition, float4x4 lightDepthViewProj[6], float4x4 lightDepthViewProjInv[6], int lightIndex, float lightCameraZNear, float lightCameraZFar)
{
	float3 lightToFrag = surfacePoint.worldPosition - lightPosition;
	float3 lightToFragDirection;
    float lightToFragDistance;
	normalizeVector(lightToFrag, lightToFragDirection, lightToFragDistance);

	float3 cubeNormals[6] = {
		{ 1.0, 0.0, 0.0},
		{-1.0, 0.0, 0.0},

		{0.0,  1.0, 0.0},
		{0.0, -1.0, 0.0},

		{0.0, 0.0,  1.0},
		{0.0, 0.0, -1.0},
	};

	float maxDot = -1;
	int faceIndex;
	for (int face = 0; face < 6; face++)
	{
		float dotVal = dot(lightToFragDirection, cubeNormals[face]);
		if (dotVal > maxDot)
		{
			maxDot = dotVal;
			faceIndex = face;
		}
	}

	float4 wPos4 = float4(surfacePoint.worldPosition, 1.0);
	float4 posInLightClipSpace = mul(wPos4, lightDepthViewProj[faceIndex]);
	posInLightClipSpace /= posInLightClipSpace.w;

	float2 textureSize;
	int elems;
	g_depthPointLight.GetDimensions(textureSize.x, textureSize.y, elems);

	float texelSize = calculateTexelSizeForPointLightShadow(textureSize.x, posInLightClipSpace.z, lightDepthViewProjInv[faceIndex], lightCameraZNear, lightCameraZFar);
	
	float3 offset = texelSize * sqrt(2.0) / 2.0 * (surfacePoint.macroNormal - 0.9 * -lightToFragDirection * dot(surfacePoint.macroNormal, -lightToFragDirection));
	
    float distanceFromCamera;
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
	distanceFromCamera = length(surfacePoint.worldPosition);
#else
    distanceFromCamera = distance(surfacePoint.worldPosition, g_cameraPosition);
#endif
	
    float4 worldPos4 = float4(surfacePoint.worldPosition + offset * max(1.0, ceil(distanceFromCamera)) * 0.35f, 1.0);

	posInLightClipSpace = mul(worldPos4, lightDepthViewProj[faceIndex]);
	posInLightClipSpace /= posInLightClipSpace.w;

	float3x3 rotMatrix = basisFromDir(lightToFragDirection);

	float3 samples[4] =
	{
        float3(texelSize, 0.0, 1.0),
		float3(-texelSize, 0.0, 1.0),
		float3(0.0,  texelSize, 1.0),
		float3(0.0, -texelSize, 1.0)
    };

	float visibility = 0.0;
	for (int j = 0; j < 4; j++)
	{
        float3 sampleDir = normalize(mul(samples[j], rotMatrix));
        visibility += g_depthPointLight.SampleCmp(g_depthSampler, float4(sampleDir, lightIndex), posInLightClipSpace.z);
    }

	visibility *= 0.25;
	visibility = smoothstep(0.33, 1.0, visibility);

	return visibility;
}

float calculateTexelSizeForSpotLightShadow(float textureSize, float depth, float4x4 depthViewProjInv)
{
	float4  firstTexelCenter_CS = { (1.0 / textureSize) - 1.0,	(1.0 / textureSize) - 1.0, depth, 1.0 };
	float4 secondTexelCenter_CS = { (3.0 / textureSize) - 1.0,	(1.0 / textureSize) - 1.0, depth, 1.0 };

	float4 firstTexelCenter_WS = mul(firstTexelCenter_CS, depthViewProjInv);
	firstTexelCenter_WS /= firstTexelCenter_WS.w;

	float4 secondTexelCenter_WS = mul(secondTexelCenter_CS, depthViewProjInv);
	secondTexelCenter_WS /= secondTexelCenter_WS.w;

	return abs(firstTexelCenter_WS.x - secondTexelCenter_WS.x);
}
float calculateVisibilityForSpotLight(SurfacePoint surfacePoint, float3 lightPosition, float4x4 lightDepthViewProj, float4x4 lightDepthViewProjInv)
{
	float3 lightToFrag = surfacePoint.worldPosition - lightPosition;
    float3 lightToFragDirection;
    float lightToFragDistance;
	normalizeVector(lightToFrag, lightToFragDirection, lightToFragDistance);

	float4 wPos4 = float4(surfacePoint.worldPosition, 1.0);
	float4 posInLightClipSpace = mul(wPos4, lightDepthViewProj);
	posInLightClipSpace /= posInLightClipSpace.w;

	float2 textureSize;
	g_depthSpotLight.GetDimensions(textureSize.x, textureSize.y);

	float texelSize = calculateTexelSizeForSpotLightShadow(textureSize.x, posInLightClipSpace.z, lightDepthViewProjInv);

	float3 offset = texelSize * sqrt(2.0) / 2.0 * (surfacePoint.macroNormal - 0.9 * -lightToFragDirection * dot(surfacePoint.macroNormal, -lightToFragDirection));
	float4 worldPos4 = float4(surfacePoint.worldPosition + offset, 1.0);

	posInLightClipSpace = mul(worldPos4, lightDepthViewProj);
	posInLightClipSpace /= posInLightClipSpace.w;

	float2 uvShadow = (posInLightClipSpace * 0.5 + 0.5).xy;
	uvShadow.y = 1.0 - uvShadow.y;

	float2 uvOffset[4] = {
		{-0.5 / textureSize.x,  0.0},
		{ 0.5 / textureSize.x,  0.0},
		{ 0.0, -0.5 / textureSize.x},
		{ 0.0,  0.5 / textureSize.x}
	};
	float visibility = 0.0;
	for (int i = 0; i < 4; i++)
	{
		visibility += g_depthSpotLight.SampleCmp(g_depthSampler, uvShadow + uvOffset[i], posInLightClipSpace.z);
	}

	visibility *= 0.25;
	visibility = smoothstep(0.33, 1.0, visibility);

	return visibility;
}

#endif