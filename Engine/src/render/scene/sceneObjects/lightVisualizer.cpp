#pragma once
#include "../scene.h"

namespace Engine
{
    bool Scene::LightVisualizer::isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial)
    {
        if (math::Sphere::isIntersecting(ray, outNearest))
        {
            ref.object = this;
            ref.type = Scene::IntersectedType::LightVisualizer;
            outMaterial = material;
            return true;
        }
        return false;
    }
}