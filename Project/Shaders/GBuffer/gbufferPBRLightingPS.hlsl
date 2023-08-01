#include "../globals.hlsl"
#include "../brdf.hlsl"
#include "../litShading.hlsl"

struct vs_out
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEX;
};

Texture2D g_albedo : TEXTURE : register(t20);
Texture2D g_roughness_metalness : TEXTURE : register(t21);
Texture2D g_normal : TEXTURE : register(t22);
Texture2D g_emission : TEXTURE : register(t23);
Texture2D g_objectID : TEXTURE : register(t24);
Texture2D g_depth : TEXTURE : register(t25);

float4 main(vs_out input) : SV_TARGET
{
    float4 color = g_albedo.Sample(g_pointWrap, input.texCoord);
    
    float2 roughness_metalness = g_roughness_metalness.Sample(g_pointWrap, input.texCoord).rg;
    float roughness = roughness_metalness.r;
    float metalness = roughness_metalness.g;
    
    float4 normalTextureGeometry = g_normal.Sample(g_pointWrap, input.texCoord);
    float3 textureNormal = unpackOctahedron(normalTextureGeometry.rg);
    float3 geometryNormal = unpackOctahedron(normalTextureGeometry.ba);

    float depth = g_depth.Sample(g_pointWrap, input.texCoord);
    
    float2 xy = input.pos.xy / float2(g_gbufferTexturesSize.x, g_gbufferTexturesSize.y);
    xy.y = 1.0 - xy.y;
    xy = (xy - 0.5) * 2.0;
    float4 posCS = float4(xy, depth, 1.0);
    float4 posWS = mul(posCS, g_viewProjInv);
    posWS /= posWS.w;
    
    SurfacePoint surfacePoint;
    surfacePoint.worldPosition = posWS / posWS.w;
    surfacePoint.macroNormal = geometryNormal;
    surfacePoint.microNormal = textureNormal;
    surfacePoint.material.color = color;
    surfacePoint.material.roughness = roughness;
    surfacePoint.material.roughness2 = pow(surfacePoint.material.roughness, 2);
    surfacePoint.material.roughness4 = pow(surfacePoint.material.roughness2, 2);
    surfacePoint.material.metalness = metalness;
    surfacePoint.material.F0 = lerp(0.04, surfacePoint.material.color, surfacePoint.material.metalness);
    
    float3 viewDirection = getViewDirection(surfacePoint.worldPosition);

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
            float3 diffuseIBL = g_diffuseIBL.Sample(g_bilinearWrap, surfacePoint.microNormal).rgb * surfacePoint.material.color.rgb * (1 - surfacePoint.material.metalness);
            result += diffuseIBL;
        }

        if (g_useSpecularReflection)
        {
            float NdotV = dot(surfacePoint.microNormal, viewDirection);
            float3 factorIBL = g_factorIBL.Sample(g_bilinearWrap, float2(surfacePoint.material.roughness, max(0.0, NdotV)));
            
            float3 r = reflect(-viewDirection, surfacePoint.microNormal);

            uint mipLevel = g_specularIBLTextureMipLevels * surfacePoint.material.roughness;
            float3 specularIBL = g_specularIBL.SampleLevel(g_trilinearWrap, r, mipLevel).rgb * (surfacePoint.material.F0 * factorIBL.r + factorIBL.g);

            result += specularIBL;
        }
    }
    
    result += g_emission.Sample(g_pointWrap, input.texCoord);
    
    return float4(result, surfacePoint.material.color.w);
}