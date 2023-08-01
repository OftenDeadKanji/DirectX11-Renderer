#pragma once
#include "../../math/mathUtils.h"

namespace Engine
{
	struct Material
	{
		Material() = default;
		Material(const math::Vec3f& albedoNormalized, float shininess, bool isEmissive = false)
			: albedo(albedoNormalized), shininess(shininess), isEmissive(isEmissive)
		{}

		math::Vec3f albedo;
		float shininess;
		bool isEmissive;
	};
}