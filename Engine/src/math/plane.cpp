#include "plane.h"
#include "ray.h"
#include "intersection.h"
#include <limits>

namespace Engine::math
{
	bool Plane::isIntersecting(const Ray& ray, Intersection& outNearest) const
	{
		float denominator = normal.dot(ray.direction);
		if (std::abs(denominator) > std::numeric_limits<float>::epsilon())
		{
			float t = (point - ray.origin).dot(normal) / denominator;
			if (t >= 0.0f && t < outNearest.t)
			{
				outNearest.t = t;
				outNearest.position = ray(t);
				outNearest.normal = normal;

				return true;
			}
		}

		return false;
	}
}