#include "../scene.h"

namespace Engine
{
    bool Scene::Plane::isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial)
    {
        if (math::Plane::isIntersecting(ray, outNearest))
        {
            ref.object = this;
            ref.type = Scene::IntersectedType::Plane;
            outMaterial = material;
            return true;
        }
        return false;
    }
}