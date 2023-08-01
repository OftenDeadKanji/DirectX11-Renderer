#include "triangle.h"
#include "ray.h"
#include "intersection.h"
#include <limits>

namespace Engine::math
{
    bool Triangle::isIntersecting(const Ray& ray, Intersection& outNearest) const
    {
        float denominator = normal.dot(ray.direction);

        if (std::abs(denominator) > std::numeric_limits<float>::epsilon())
        {
            Vertex& v0 = *(verticesArray + vertexIndices[0]);

            float D = -normal.dot(v0.position);
            float t = -(normal.dot(ray.origin) + D) / denominator;

            if (t >= 0.0f && t < outNearest.t)
            {
                Vec3f P = ray(t);

                Vertex& v1 = *(verticesArray + vertexIndices[1]);
                Vertex& v2 = *(verticesArray + vertexIndices[2]);

                Vec3f edge01 = v0.position - v1.position;
                Vec3f edge12 = v1.position - v2.position;
                Vec3f edge20 = v2.position - v0.position;

                Vec3f C0 = P - v0.position;
                Vec3f C1 = P - v1.position;
                Vec3f C2 = P - v2.position;

                if (normal.dot(edge01.cross(C0)) > 0 &&
                    normal.dot(edge12.cross(C1)) > 0 &&
                    normal.dot(edge20.cross(C2)) > 0)
                {
                    outNearest.t = t;
                    outNearest.position = P;
                    outNearest.normal = normal;

                    return true;
                }
            }
        }

        return false;
    }

    bool math::Triangle::isIntersecting(const Ray& ray, MeshIntersection& outNearest) const
    {
        math::Intersection inter;
        inter.t = outNearest.t;
        inter.position = outNearest.position;
        inter.normal = outNearest.normal;

        if (isIntersecting(ray, inter))
        {
            outNearest.t = inter.t;
            outNearest.position = inter.position;
            outNearest.normal = inter.normal;

            return true;
        }

        return false;
    }
}