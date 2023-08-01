#include "../globals.hlsl"
#include "fog.hlsl"

struct vs_out
{
    float4 pos_clip : SV_Position;
    float3 modelPos : MODPOS;
    float3 worldPos : WPOS;
    float4x4 modelToWorld : MOD;
    float4x4 worldToModel : MODINV;
};

Texture2D g_sceneColor : TEXTURE : register(t20);
Texture2D g_sceneDepth : TEXTURE : register(t21);
Texture3D g_fogTexture : TEXTURE : register(t22);

float3 collision(float3 origin, float3 direction, float3 minPoint, float3 maxPoint)
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

    float t = max(tmin, tmax);
    return origin + direction * t;
}

float4 main(vs_out input) : SV_TARGET
{
    float3 sceneColor = g_sceneColor.Load(int3(input.pos_clip.xy, 0)).rgb;
    float sceneDepth = g_sceneDepth.Load(int3(input.pos_clip.xy, 0)).r;
    
    float4 sceneCS = float4(input.pos_clip.xy, sceneDepth, 1.0);
    sceneCS.xy /= g_gbufferTexturesSize;
    sceneCS.y = 1.0 - sceneCS.y;
    sceneCS.xy = (sceneCS.xy - 0.5) * 2.0;
    
    float4 sceneWS = mul(sceneCS, g_viewProjInv);
    sceneWS /= sceneWS.w;
    
    float3 startPos = input.worldPos;
    float3 viewDirection = getViewDirection(startPos);
    
    float3 minPoint = mul(float4(-1.0, -1.0, -1.0, 1.0), input.modelToWorld).xyz;
    float3 maxPoint = mul(float4(1.0, 1.0, 1.0, 1.0), input.modelToWorld).xyz;
    
    float3 endPos = collision(startPos, viewDirection, minPoint, maxPoint);
    
    float distanceInBox = distance(startPos, endPos);
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
    float3 cameraPos = float3(0.0, 0.0, 0.0);
#else
    float3 cameraPos = g_cameraPosition;
#endif
    
    float distanceSceneToCamera = distance(sceneWS.xyz, cameraPos);
    float distanceStartPointToCamera = distance(startPos, cameraPos);
    float distanceEndPointToCamera = distance(endPos, cameraPos);
    float distanceStartToEnd = distance(startPos, endPos);
    
    //Scene -- Start -- End -- Camera
    // default behaviour
    
    //Start -- Scene -- End ? Camera
    if(distanceSceneToCamera < distanceStartPointToCamera)
    {
        startPos = sceneWS.xyz;
    }
    
    //Start ? Scene -- Camera -- End
    if(distanceStartPointToCamera < distanceStartToEnd)
    {
        endPos = cameraPos;
    }
    
    // Start -- End -- Scene -- Camera
    if(distanceStartToEnd < distanceStartPointToCamera && distanceSceneToCamera < distanceEndPointToCamera)
    {
        return float4(sceneColor, 1.0);
    }
    
    const int steps = 20;
    float dt = distance(startPos, endPos) / steps;
    
    float3 fog;
    float cosTheta = dot(-g_directionalLight.direction, viewDirection);
    
    float3 color = sceneColor;
    for (int i = 0; i < steps; i++)
    {
        float3 pos = startPos + dt * i;
        float3 uv = mul(float4(pos, 1.0), input.worldToModel).xyz;
        uv = (uv + 1) * 0.5;
        
        float density = g_fogTexture.SampleLevel(g_pointWrap, uv, 0);
    
        float F_ex = exp(-density * dt);
        float3 L_ex = color * F_ex;
    
        float phaseFunctionVal = phaseFunction(cosTheta, g_phaseFunctionParameter);
    
        float3 lightEnergy = g_directionalLight.energy * g_directionalLight.solidAngle;
        
        float3 lightBound = collision(pos, g_directionalLight.direction, minPoint, maxPoint);
        float lightDist = distance(pos, lightBound);
        float dtL = lightDist / steps;
        for (int j = 0; j < steps; j++)
        {
            float3 posL = startPos + dt * i;
            float3 uvL = mul(float4(posL, 1.0), input.worldToModel).xyz;
            uvL = (uvL + 1) * 0.5;
        
            float densityL = g_fogTexture.SampleLevel(g_pointWrap, uvL, 0);
            
            lightEnergy = exp(-densityL * dtL);
        }
        
            float3 L_in = (1.0 / max(density, 0.0001)) * lightEnergy * density * min(phaseFunctionVal, 1.0) * (1.0 - exp(-density * dt));
    
        color = L_ex + L_in;
    }
    //fog = steps;
    
    return float4(color, 1.0);
}