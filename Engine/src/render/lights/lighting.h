#pragma once
#include "../../math/mathUtils.h"
#include "../material/material.h"
#include "directionalLight.h"
#include "pointLight.h"
#include "spotLight.h"

namespace Engine
{
	math::Vec3f calculateLighting_DirectionalLight(const DirectionalLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial);
	math::Vec3f calculateLighting_PointLight(const PointLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial);
	math::Vec3f calculateLighting_SpotLight(const SpotLight& light, const math::Vec3f& cameraPosition, const math::Vec3f& surfacePosition, const math::Vec3f& surfaceNormal, const Material& surfaceMaterial);
}