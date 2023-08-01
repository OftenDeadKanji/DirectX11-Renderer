#pragma once
#include "../dependencies/Eigen/Eigen/Dense"
#include "mathUtils.h"

namespace Engine::math
{
	struct Ray;
	struct Intersection;
	
	struct Sphere
	{
		math::Vec3f position;
		float radius;

		bool isIntersecting(const Ray& ray, Intersection& outNearest) const;
	};
}