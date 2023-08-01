#include "../../math/ray.h"
#include "../../math/intersection.h"
#include "../scene.h"

namespace Engine
{
	bool Scene::MeshInstance::isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial)
	{
		math::Ray rayInMS;
		rayInMS.origin = (math::Vec4f(ray.origin.x(), ray.origin[1], ray.origin[2], 1.0f) * transform.transformInvMat).head<3>();
		rayInMS.direction = (math::Vec4f(ray.direction.x(), ray.direction[1], ray.direction[2], 0.0f) * transform.transformInvMat).head<3>().normalized();

		math::MeshIntersection inter;
		if (outNearest.valid())
		{
			inter.position = (math::Vec4f(outNearest.position.x(), outNearest.position.y(), outNearest.position.z(), 1.0f) * transform.transformInvMat).head<3>();
			inter.t = (inter.position - rayInMS.origin).norm();
		}
		else
		{
			inter.reset(0.0f);
		}

		//for (auto& triangle : mesh->triangles)
		//{
			if (mesh->octree.intersect(rayInMS, inter))
			{
				outNearest.position = (math::Vec4f(inter.position[0], inter.position[1], inter.position[2], 1.0f) * transform.transformMat).head<3>();
				outNearest.normal = (math::Vec4f(inter.normal[0], inter.normal[1], inter.normal[2], 0.0f) * transform.transformMat).head<3>().normalized();
				outNearest.t = (outNearest.position - ray.origin).norm();

				outMaterial = material;

				ref.object = this;
				ref.type = Scene::IntersectedType::Mesh;

				return true;
			}
		//}
		return false;
	}
}