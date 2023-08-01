#pragma once
#include "../../math/mathUtils.h"

namespace Engine
{
	struct Particle
	{
		math::Vec4f color;
		math::Vec3f position;
		float rotationAngle;
		math::Vec2f size;
		math::Vec3f speed;

		float lifetime;
		float maxLifeTime;
	};
}