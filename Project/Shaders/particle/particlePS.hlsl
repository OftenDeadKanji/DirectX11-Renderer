#include "../globals.hlsl"
#include "../fog/fog.hlsl"

struct vs_out
{
	float4 position : SV_POSITION;
	float3 worldPos : POS;
	float3x3 basis : BS;
	uint frameIndex : FIDX;
	float frameFraction : FFRAC;
	float4 color : COL;
	float2 texCoord : TEX;
};

cbuffer SmokeAtlas : register(b10)
{
	int2 g_atlasSize;
};

Texture2D g_particleRLTTexture : TEXTURE: register(t20);
Texture2D g_particleBBFTexture : TEXTURE: register(t21);
Texture2D g_particleEMVATexture : TEXTURE: register(t22);

Texture2D<float4> g_sceneDepthTexture : TEXTURE: register(t23);

Texture3D g_fogTexture : TEXTURE : register(t24);

float2 calculateUV(float2 inputUV, uint frameIndex)
{
	int2 atlasCoord = {
		fmod(frameIndex, g_atlasSize.x),
		frameIndex / g_atlasSize.x
	};

	float2 singleFrameTextureSize = 1.0.xx / g_atlasSize;

	return inputUV / g_atlasSize + atlasCoord * singleFrameTextureSize;
}
void getParticleTextureData(out float3 rtl, out float3 bbf, out float2 ea, vs_out input)
{
	float2 uvThis = calculateUV(input.texCoord, input.frameIndex);
	float2 uvNext = calculateUV(input.texCoord, input.frameIndex + 1);

	float2 mv0 = 2.0 * g_particleEMVATexture.Sample(g_bilinearWrap, uvThis).gb - 1.0;
	float2 mv1 = 2.0 * g_particleEMVATexture.Sample(g_bilinearWrap, uvNext).gb - 1.0;

	mv0.y = -mv0.y;
	mv1.y = -mv1.y;

	static const float MV_SCALE = 0.0015;
	float time = input.frameFraction;

	float2 uv0 = uvThis;
	uv0 -= mv0 * MV_SCALE * time;

	float2 uv1 = uvNext;
	uv1 -= mv1 * MV_SCALE * (time - 1.0);

	float3 rtlTexVal0 = g_particleRLTTexture.Sample(g_bilinearWrap, uv0);
	float3 rtlTexVal1 = g_particleRLTTexture.Sample(g_bilinearWrap, uv1);

	rtl = lerp(rtlTexVal0, rtlTexVal1, time);

	float3 bbfTexVal0 = g_particleBBFTexture.Sample(g_bilinearWrap, uv0);
	float3 bbfTexVal1 = g_particleBBFTexture.Sample(g_bilinearWrap, uv1);

	bbf = lerp(bbfTexVal0, bbfTexVal1, time);

	float2 eaTexVal0 = g_particleEMVATexture.Sample(g_bilinearWrap, uv0).ra;
	float2 eaTexVal1 = g_particleEMVATexture.Sample(g_bilinearWrap, uv1).ra;

	ea = lerp(eaTexVal0, eaTexVal1, time);
}
float3 calculateParticleLighting(float3 irradiance, float factor, float3 basis, float3 lightDirection)
{
	return irradiance * factor * max(0, dot(basis, lightDirection));
}

float3 calculateParticleLightingForDirectionalLight(DirectionalLight light, float factor, float3 basis)
{
	float3 irradiance = light.energy * light.solidAngle;

	float3 dirLightValue = calculateParticleLighting(irradiance, factor, basis, -light.direction);

	return dirLightValue;
}

float3 calculateParticleLightingForPointLight(PointLight light, float factor, float3 basis, float3 worldPos)
{
	float3 fragToLight = light.position - worldPos;
    float3 fragToLightNormalized;
	float fragToLightLength;

	normalizeVector(fragToLight, fragToLightNormalized, fragToLightLength);

	float solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - min(1.0, pow(light.radius / fragToLightLength, 2.0))));
	float irradiance = light.energy * solidAngle;

	float3 pointLightValue = calculateParticleLighting(irradiance, factor, basis, fragToLightNormalized);

	return pointLightValue;
}

float3 calculateParticleLightingForSpotLight(SpotLight light, float factor, float3 basis, float3 worldPos)
{
	float3 fragToLight = light.position - worldPos;
    float3 fragToLightNormalized;
	float fragToLightLength;

	normalizeVector(fragToLight, fragToLightNormalized, fragToLightLength);

	float cosToFrag = dot(-fragToLightNormalized, light.direction);
	float visible = cosToFrag - light.cosAngle >= 0.0 ? 1.0 : 0.0;

	float solidAngle = 2.0 * PI * (1.0 - sqrt(1.0 - min(1.0, pow(light.radius / fragToLightLength, 2.0))));
	float irradiance = light.energy * solidAngle;

	float3 spotLightValue = calculateParticleLighting(irradiance, factor, basis, fragToLightNormalized);

	float4 posBySpotLight = mul(float4(worldPos, 1.0), light.projectionMatrix);
	float2 spotUV = posBySpotLight.xy / posBySpotLight.w;
	spotUV.y *= -1.0;

	float4 spotLightTexColor = g_spotLightTextue.Sample(g_bilinearWrap, spotUV);

	spotLightValue *= (1.0 - spotLightTexColor.w) * visible;

	return spotLightValue;
}

void smoothClipping(inout float particleAlpha, vs_out input, float particleThickness)
{
    float storedDepth = g_sceneDepthTexture.Load(int3(input.position.xy, 0));
	storedDepth = g_zNear * g_zFar / (g_zFar + storedDepth * (g_zNear - g_zFar));

	float currentDepth = g_zNear * g_zFar / (g_zFar + input.position.z * (g_zNear - g_zFar));

	float nearObjectDepthDiff = max(0.0, storedDepth.x - currentDepth);
	float nearObjectDepthT = min(1.0, nearObjectDepthDiff / particleThickness);
	float nearObjectAlpha = lerp(0.0, particleAlpha, nearObjectDepthT);

	float nearPlaneDepthT = min(1.0, currentDepth / particleThickness);
	float nearPlaneAlpha = lerp(0.0, particleAlpha, nearPlaneDepthT);

	particleAlpha = min(nearObjectAlpha, nearPlaneAlpha);
}

bool collision(float3 origin, float3 direction, float3 minPoint, float3 maxPoint, out float3 startPoint, out float3 endPoint)
{
    float tmin = (minPoint.x - origin.x) / direction.x;
    float tmax = (maxPoint.x - origin.x) / direction.x;

    if (tmin > tmax)
    {
        float t = tmin;
        tmin = tmax;
        tmax = t;
    }

    float tymin = (minPoint.y - origin.y) / direction.y;
    float tymax = (maxPoint.y - origin.y) / direction.y;

    if (tymin > tymax)
    {
        float t = tymin;
        tymin = tymax;
        tymax = t;
    }

    if (tmin > tymax || tymin > tmax)
    {
        return false;
    }

    if (tymin > tmin)
    {
        tmin = tymin;
    }

    if (tymax < tmax)
    {
        tmax = tymax;
    }

    float tzmin = (minPoint.z - origin.z) / direction.z;
    float tzmax = (maxPoint.z - origin.z) / direction.z;

    if (tzmin > tzmax)
    {
        float t = tzmin;
        tzmin = tzmax;
        tzmax = t;
    }

    if (tmin > tzmax || tzmin > tmax)
    {
        return false;
    }

    if (tzmin > tmin)
    {
        tmin = tzmin;
    }

    if (tzmax < tmax)
    {
        tmax = tzmax;
    }

    startPoint = origin + max(tmin, 0.0) * direction;
    endPoint = origin + max(tmax, 0.0) * direction;
	
    return true;
}

float4 main(vs_out input) : SV_TARGET
{
	float3 rtlVal, bbfVal;
	float2 eaVal;
	getParticleTextureData(rtlVal, bbfVal, eaVal, input);

	float factor[6] = {
		rtlVal.r, rtlVal.g, rtlVal.b,		// right - top - left
		bbfVal.r, bbfVal.g, bbfVal.b		// bottom - back - front
	};

	float3 basis[6] = {
		 input.basis[0],  input.basis[1], -input.basis[0],
		-input.basis[1], -input.basis[2],  input.basis[2]
	};

	float4 color = input.color;
	float3 lighting = { 0.0, 0.0, 0.0 };
	for (int i = 0; i < 6; i++)
	{
		// dir light
		lighting += calculateParticleLightingForDirectionalLight(g_directionalLight, factor[i], basis[i]);

		// point light
		for (int j = 0; j < g_pointLightsCount; j++)
		{
			lighting += calculateParticleLightingForPointLight(g_pointLights[j], factor[i], basis[i], input.worldPos);
		}

		// spot light
		lighting += calculateParticleLightingForSpotLight(g_spotLight, factor[i], basis[i], input.worldPos);
	}

	color.rgb *= lighting + eaVal.x;
	color.a *= eaVal.y;

	static const float PARTICLE_THICKNESS = 1.0;
	smoothClipping(color.a, input, PARTICLE_THICKNESS);

    float3 viewDirection = getViewDirection(input.worldPos);
	
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float3 cameraPos = float3(0.0, 0.0, 0.0);
#else
    float3 cameraPos = g_cameraPosition;
#endif
	
	if(g_uniformFogEnabled)
    {
        float3 dirLightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
        float d = g_zNear * g_zFar / (g_zFar + input.position.z * (g_zNear - g_zFar));
        float cosTheta = dot(viewDirection, -g_directionalLight.direction);
	
        float3 colorAfterFog = calculateFog(color.rgb, dirLightEnergy, d, g_fogDensity, g_phaseFunctionParameter, cosTheta);
		
        return float4(colorAfterFog, color.w);
    }
	else if(g_volumetricFogEnabled)
    {
        for (int fog = 0; fog < g_volumetricFogInstancesCount; fog++)
        {
            float3 minPoint = mul(float4(-1, -1, -1, 1), g_volumetricFogInstances[fog].fogToWorld).xyz;
            float3 maxPoint = mul(float4(1, 1, 1, 1), g_volumetricFogInstances[fog].fogToWorld).xyz;
			
            float3 startPoint, endPoint;
			
            bool isCollision = collision(input.worldPos, viewDirection, minPoint, maxPoint, startPoint, endPoint);
            if(!isCollision)
            {
                continue;
            }
            
            float distanceInBox = distance(startPoint, endPoint);
            float3 distanceToCamera = distance(input.worldPos, cameraPos);
            float minDistance = min(distanceInBox, distanceToCamera);
			
            float distanceParticleToCamera = distance(input.worldPos, cameraPos);
            if (distanceParticleToCamera < distance(cameraPos, endPoint))
            {
                continue;
            }
    
            if (distanceParticleToCamera < distance(cameraPos, startPoint))
            {
                startPoint = input.worldPos;
            }
			
            const int steps = 20;
            float dt = minDistance / steps;
    
            float cosTheta = dot(-g_directionalLight.direction, viewDirection);
    
            //float3 color = sceneColor;
            for (int i = 0; i < steps; i++)
            {
                float3 pos = startPoint + dt * i;
                float3 uv = mul(float4(pos, 1.0), g_volumetricFogInstances[fog].worldToFog).xyz;
                uv = (uv + 1) * 0.5;
        
                float density = g_fogTexture.SampleLevel(g_pointWrap, uv, 0);
    
                float F_ex = exp(-density * dt);
                float3 L_ex = color.rgb * F_ex;
    
                float phaseFunctionVal = phaseFunction(cosTheta, g_phaseFunctionParameter);
    
                float3 lightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
        
                float3 p, lightBound;
                collision(pos, g_directionalLight.direction, minPoint, maxPoint, p, lightBound);
                
                float lightDist = distance(pos, lightBound);
                float dtL = lightDist / steps;
                for (int j = 0; j < steps; j++)
                {
                    float3 posL = startPoint + dt * i;
                    float3 uvL = mul(float4(posL, 1.0), g_volumetricFogInstances[fog].worldToFog).xyz;
                    uvL = (uvL + 1) * 0.5;
        
                    float densityL = g_fogTexture.SampleLevel(g_pointWrap, uvL, 0);
            
                    lightEnergy = exp(-densityL * dtL);
                }
        
                float3 L_in = (1.0 / max(density, 0.0001)) * lightEnergy * density * min(phaseFunctionVal, 1.0) * (1.0 - exp(-density * dt));
    
                color.rgb = L_ex + L_in;
            }
        }
    }
	
    return color;
}