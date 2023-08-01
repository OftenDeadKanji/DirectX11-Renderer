#include "../globals.hlsl"
#include "../fog/fog.hlsl"

struct vs_out
{
    float4 position : SV_POSITION;
    float3 worldPos : POS;
    float3x3 basis : BS;
    float4 color : COL;
    float2 texCoord : TEX;
};

Texture2D<float4> g_particleTexture : TEXTURE : register(t20);
Texture2D<float4> g_sceneDepthTexture : TEXTURE : register(t23);
Texture3D g_fogTexture : TEXTURE : register(t24);

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
    float4 color = input.color;
    color *= g_particleTexture.Sample(g_bilinearWrap, input.texCoord);

    static const float PARTICLE_THICKNESS = 0.2;
    smoothClipping(color.a, input, PARTICLE_THICKNESS);

    float3 viewDirection = getViewDirection(input.worldPos);
	
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float3 cameraPos = float3(0.0, 0.0, 0.0);
#else
    float3 cameraPos = g_cameraPosition;
#endif
	
    if (g_uniformFogEnabled)
    {
        float3 dirLightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
        float d = g_zNear * g_zFar / (g_zFar + input.position.z * (g_zNear - g_zFar));
        float cosTheta = dot(viewDirection, -g_directionalLight.direction);
	
        float3 colorAfterFog = calculateFog(color.rgb, dirLightEnergy, d, g_fogDensity, g_phaseFunctionParameter, cosTheta);
		
        return float4(colorAfterFog, color.w);
    }
    else if (g_volumetricFogEnabled)
    {
        for (int fog = 0; fog < g_volumetricFogInstancesCount; fog++)
        {
            float3 minPoint = mul(float4(-1, -1, -1, 1), g_volumetricFogInstances[fog].fogToWorld).xyz;
            float3 maxPoint = mul(float4(1, 1, 1, 1), g_volumetricFogInstances[fog].fogToWorld).xyz;
			
            float3 startPoint, endPoint;
			
            bool isCollision = collision(input.worldPos, viewDirection, minPoint, maxPoint, startPoint, endPoint);
            if (!isCollision)
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