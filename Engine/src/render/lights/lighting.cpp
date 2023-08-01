#include "lighting.h"

namespace Engine
{
    math::Vec3f BlinnPhong(const math::Vec3f& viewDirection, const math::Vec3f& lightDirection, const math::Vec3f& lightColor, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial)
    {
        math::Vec3f h = (lightDirection + viewDirection).normalized();

        float diffuseFactor = std::max((lightDirection).dot(surfaceNormal), 0.0001f);
        float specularFactor = std::powf(std::max(h.dot(surfaceNormal), 0.0001f), surfaceMaterial.shininess);

        return surfaceMaterial.albedo.cwiseProduct(lightColor) * diffuseFactor + lightColor * specularFactor;
    }

    math::Vec3f calculateLighting_DirectionalLight(const DirectionalLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial)
    {
        math::Vec3f cameraDirection = (cameraPosition - surfacePosition).normalized();

        return BlinnPhong(cameraDirection, -light.getDirection(), light.getColor(), surfaceNormal, surfaceMaterial);
    }

    math::Vec3f calculateLighting_PointLight(const PointLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial)
    {
        math::Vec3f lightDirection = (light.getPos() - surfacePosition).normalized();
        math::Vec3f cameraDirection = (cameraPosition - surfacePosition).normalized();

        float distance = (light.getPos() - surfacePosition).norm();
        float attenuation = 1.0f / (distance * distance);

        return BlinnPhong(cameraDirection, lightDirection, light.getColor(), surfaceNormal, surfaceMaterial) * attenuation;
    }

    math::Vec3f calculateLighting_SpotLight(const SpotLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial)
    {
        math::Vec3f cameraDirection = (cameraPosition - surfacePosition).normalized();
        math::Vec3f lightDirection = (light.getPos() - surfacePosition).normalized();

        if (light.isInside(surfacePosition))
        {
            float distance = (light.getPos() - surfacePosition).norm();
            float attenuation = 1.0f / (distance * distance);

            return BlinnPhong(cameraDirection, lightDirection, light.getColor(), surfaceNormal, surfaceMaterial) * attenuation;
        }

        return math::Vec3f(0.0f, 0.0f, 0.0f);
    }
}