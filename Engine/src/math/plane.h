#pragma once
#include "mathUtils.h"

namespace Engine::math
{
	struct Ray;
	struct Intersection;

	struct Plane
	{
		Plane() = default;
		Plane(const Vec3f& point, const Vec3f& normal)
			: point(point), normal(normal)
		{}

		Vec3f point;
		Vec3f normal;

		bool isIntersecting(const Ray& ray, Intersection& outNearest) const;
	};
}