#include "../scene.h"

namespace Engine
{
    bool Scene::Sphere::isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial)
    {
        if (math::Sphere::isIntersecting(ray, outNearest))
        {
            ref.object = this;
            ref.type = Scene::IntersectedType::Sphere;
            outMaterial = material;
            return true;
        }
        return false;
    }
}