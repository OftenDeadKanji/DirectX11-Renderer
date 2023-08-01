#pragma once
#include "mathUtils.h"

namespace Engine::math
{
	struct Ray
	{
		math::Vec3f origin;
		math::Vec3f direction;

		math::Vec3f operator()(float t) const 
		{
			return origin + direction * t;
		}

		math::Vec3f at(float t) const
		{
			return operator()(t);
		}
	};
}