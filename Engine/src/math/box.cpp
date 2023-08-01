#include "box.h"
#include "ray.h"
#include "intersection.h"

namespace Engine::math
{
	bool Box::intersects(const Ray& ray, Intersection& outNearest) const
	{
		float tmin = (min.x() - ray.origin.x()) / ray.direction.x();
		float tmax = (max.x() - ray.origin.x()) / ray.direction.x();

		if (tmin > tmax)
		{
			std::swap(tmin, tmax);
		}

		float tymin = (min.y() - ray.origin.y()) / ray.direction.y();
		float tymax = (max.y() - ray.origin.y()) / ray.direction.y();

		if (tymin > tymax)
		{
			std::swap(tymin, tymax);
		}

		if (tmin > tymax || tymin > tmax)
		{
			return false;
		}

		if (tymin > tmin)
		{
			tmin = tymin;
		}

		if (tymax < tmax)
		{
			tmax = tymax;
		}

		float tzmin = (min.z() - ray.origin.z()) / ray.direction.z();
		float tzmax = (max.z() - ray.origin.z()) / ray.direction.z();

		if (tzmin > tzmax)
		{
			std::swap(tzmin, tzmax);
		}

		if (tmin > tzmax || tzmin > tmax)
		{
			return false;
		}

		if (tzmin > tmin)
		{
			tmin = tzmin;
		}

		if (tzmax < tmax)
		{
			tmax = tzmax;
		}

		float t = std::min(std::max(tmin, 0.0f), std::max(tmax, 0.0f));
		if (t > outNearest.t)
		{
			return false;
		}

		outNearest.t = t;
		outNearest.position = ray(t);

		float pX = center().x() - outNearest.position.x();
		float pY = center().y() - outNearest.position.y();
		float pZ = center().z() - outNearest.position.z();

		float pxPercentage = pX / size().x();
		float pyPercentage = pY / size().y();
		float pzPercentage = pZ / size().z();

		if (pxPercentage > pyPercentage && pxPercentage > pzPercentage)
		{
			outNearest.normal = pX > 0.0f ? math::Vec3f{ -1.0f, 0.0f, 0.0f } : math::Vec3f{ 1.0f, 0.0f, 0.0f };
		}
		else if (pyPercentage > pxPercentage && pyPercentage > pzPercentage)
		{
			outNearest.normal = pX > 0.0f ? math::Vec3f{ 0.0f, -1.0f, 0.0f } : math::Vec3f{ 0.0f, 1.0f, 0.0f };
		}
		else
		{
			outNearest.normal = pX > 0.0f ? math::Vec3f{ 0.0f, 0.0f, -1.0f } : math::Vec3f{ 0.0f, 0.0f, 1.0f };
		}

		return true;
	}

	bool Box::intersects(const Ray& ray, MeshIntersection& outNearest) const
	{
		math::Intersection inter;
		inter.position = outNearest.position;
		inter.normal = outNearest.normal;
		inter.t = outNearest.t;

		if (intersects(ray, inter))
		{
			outNearest.t = inter.t;
			outNearest.position = inter.position;
			outNearest.normal = inter.normal;

			return true;
		}
		return false;
	}
}
