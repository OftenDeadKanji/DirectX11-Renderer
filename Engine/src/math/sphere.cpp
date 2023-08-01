#include "sphere.h"
#include "ray.h"
#include "intersection.h"

namespace Engine::math
{
    bool Sphere::isIntersecting(const Ray& ray, Intersection& outNearest) const
    {
        //optimized version, source - Real Time Rendering (4th edition)
        math::Vec3f l = position - ray.origin;
        float s = l.dot(ray.direction);
        float l2 = l.dot(l);
        float r2 = radius * radius;

        if (s < 0 && l2 > r2)
        {
            return false;
        }

        float m2 = l2 - s * s;
        if (m2 > r2)
        {
            return false;
        }

        float q = std::sqrtf(r2 - m2);
        float t = l2 > r2 ? s - q : s + q;

        if (t > outNearest.t)
        {
            return false;
        }

        outNearest.t = t;
        outNearest.position = ray(t);
        outNearest.normal = (outNearest.position - position).normalized();

        return true;
    }
}