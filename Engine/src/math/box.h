#pragma once
#include "mathUtils.h"
#include <limits>

namespace Engine::math
{
	struct Ray;
	struct Intersection;
	struct MeshIntersection;

	struct Box
	{
		Vec3f min, max;

		static constexpr float Inf = std::numeric_limits<float>::infinity();
		
		static Box empty()
		{
			return { {Inf, Inf, Inf}, {-Inf, -Inf, -Inf} };
		}

		static Box unit()
		{
			return { { -1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f} };
		}

		Vec3f size() const
		{
			return max - min;
		}

		Vec3f center() const
		{
			return (min + max) / 2.0f;
		}

		float radius() const
		{
			return size().norm() / 2.0f;
		}

		void reset()
		{
			constexpr float maxf = (std::numeric_limits<float>::max)();
			min = { maxf, maxf, maxf };
			max = -min;
		}

		void expand(const Box& box)
		{
			min = min.cwiseMin(box.min);
			max = max.cwiseMax(box.max);
		}

		void expand(const Vec3f& point)
		{
			min = min.cwiseMin(point);
			max = max.cwiseMax(point);
		}

		bool contains(const Vec3f& point) const 
		{
			return
				min[0] <= point[0] && point[0] <= max[0] &&
				min[1] <= point[1] && point[1] <= max[1] &&
				min[2] <= point[2] && point[2] <= max[2];
		}

		bool intersects(const Ray& ray, Intersection& outNearest) const;
		bool intersects(const Ray& ray, MeshIntersection& outNearest) const;
		
	};
}
