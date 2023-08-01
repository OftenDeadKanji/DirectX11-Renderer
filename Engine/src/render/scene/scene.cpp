#define NOMINMAX
#include "scene.h"
#include "../../window/window.h"
#include "../camera/camera.h"
#include "../../math/ray.h"
#include "../../objectMover/sphereMover.h"
#include "../../objectMover/planeMover.h"
#include "../../objectMover/meshInstanceMover.h"
#include "../../objectMover/lightVisualizerMover.h"
#include "../lights/lighting.h"
#include "../../utils/assert.h"

namespace Engine
{
	Scene::Scene()
		: m_parallelExecutor(std::max(1u, std::max(ParallelExecutor::MAX_THREADS > 4u ? ParallelExecutor::MAX_THREADS - 4u : 1u, ParallelExecutor::HALF_THREADS)))
	{
		start = std::chrono::high_resolution_clock::now();
	}

	void Scene::addPlane(const math::Vec3f& pointOnPlane, const math::Vec3f& normal, const Material& material)
	{
		m_planes.emplace_back();

		m_planes.back().point = pointOnPlane;
		m_planes.back().normal = normal;
		m_planes.back().material = material;
	}

	void Scene::addSphere(const math::Vec3f& position, float radius, const Material& material)
	{
		m_spheres.emplace_back();

		m_spheres.back().position = position;
		m_spheres.back().radius = radius;
		m_spheres.back().material = material;
	}

	void Scene::addCube(const Transform& transform, const Material& material)
	{
		//m_cubes.emplace_back(m_meshManager.getMesh("Cube"));
		//
		//m_cubes.back().transform = transform;
		//m_cubes.back().transform.updateMat();
		//m_cubes.back().material = material;
	}

	void Scene::addDirectionalLight(const math::Vec3f& color, const math::Vec3f& direction)
	{
		m_directionalLights.emplace_back(color, direction);
	}

	void Scene::addPointLight(const math::Vec3f& color, const math::Vec3f& position)
	{
		m_pointLights.emplace_back(color, position);

		m_lightsVisualization.emplace_back();
		m_lightsVisualization.back().light = &m_pointLights.back();
		m_lightsVisualization.back().lightType = Scene::LightVisualizer::LightType::Point;
		m_lightsVisualization.back().position = m_pointLights.back().getPos();
		m_lightsVisualization.back().radius = 1.0f;
		m_lightsVisualization.back().material = Material(math::Vec3f(0.1f, 0.1f, 1.0f), 0.0f, true);
	}

	void Scene::addSpotLight(const math::Vec3f& color, const math::Vec3f& position, const math::Vec3f& direction, float angle)
	{
		m_spotLights.emplace_back(color, position, direction, angle);

		m_lightsVisualization.emplace_back();
		m_lightsVisualization.back().light = &m_spotLights.back();
		m_lightsVisualization.back().lightType = Scene::LightVisualizer::LightType::Spot;
		m_lightsVisualization.back().position = m_spotLights.back().getPos();
		m_lightsVisualization.back().radius = 1.0f;
		m_lightsVisualization.back().material = Material(math::Vec3f(1.0f, 0.1f, 0.1f), 0.0f, true);
	}

	void Scene::setAmbientLight(const math::Vec3f& color, float intensity)
	{
		m_backgroundColor = color;
		m_ambientFactor = std::clamp(intensity, 0.0f, 1.0f);
	}

	void Scene::render(Window& window, Camera& camera)
	{
		//window.clear(math::Vec3f(1.0f, 0.9f, 0.1f));

		//auto now = std::chrono::high_resolution_clock::now();
		//auto seconds = std::chrono::duration<float>(now - start).count();

		//math::Vec2f windowSize = window.getSize().cast<float>();
		//shader.updateShader(math::Vec4f(windowSize.x(), windowSize.y(), 1.0f / windowSize.x(), 1.0f / windowSize.y()), seconds);
		
		//shader.draw(vertexBuffer, indexBuffer);

#if 0
		auto bufferSize = window.getPixelBufferSize();

		auto func = [this, &window, &bufferSize, &camera](uint32_t threadIndex, uint32_t taskIndex)
		{
			computePixelColor(math::Vec2i(taskIndex % bufferSize.x(), taskIndex / bufferSize.x()), window, camera);
		};
		m_parallelExecutor.execute(func, bufferSize.x() * bufferSize.y(), 20);
#endif
	}

	bool Scene::findIntersection(const math::Ray& ray, math::Intersection& outNearest, Material& outMaterial)
	{
		ObjRef ref = { nullptr, IntersectedType::NUM };

		findIntersectionInternal(ray, ref, outNearest, outMaterial);

		return outNearest.valid();
	}

	bool Scene::findIntersection(const math::Ray& ray, IntersectionQuery& query)
	{
		ObjRef ref = { nullptr, IntersectedType::NUM };

		findIntersectionInternal(ray, ref, query.nearest, *query.material);

		switch (ref.type)
		{
		case IntersectedType::Sphere:
		{
			Sphere* sphere = static_cast<Sphere*>(ref.object);
			if (query.mover)
			{
				query.mover->reset(new SphereMover(sphere));
			}
		}
		break;
		case IntersectedType::Mesh:
		{
			MeshInstance* mesh = static_cast<MeshInstance*>(ref.object);
			if (query.mover)
			{
				query.mover->reset(new MeshInstanceMover(mesh));
			}
		}
		break;
		case IntersectedType::LightVisualizer:
		{
			LightVisualizer* lightVisualizer = static_cast<LightVisualizer*>(ref.object);
			if (query.mover)
			{
				query.mover->reset(new LightVisualizerMover(lightVisualizer));
			}
		}
		break;
		}

		return ref.type != IntersectedType::NUM;
	}

	void Scene::computePixelColor(const math::Vec2i& pixelCoordiante, Window& window, Camera& camera)
	{
		int pxX = pixelCoordiante.x();
		int pxY = pixelCoordiante.y();

		auto bufferSize = window.getPixelBufferSize();

		math::Ray ray = camera.generateRay(pixelCoordiante, bufferSize);

		math::Intersection intersection;
		intersection.reset();
		Material material;

		math::Vec3f pixelColor = m_backgroundColor;
		if (findIntersection(ray, intersection, material))
		{
			lighting(pixelColor, camera.position(), intersection.position, intersection.normal, material);
		}

		window.setPixelColor(pixelCoordiante, pixelColor);
	}

	void Scene::findIntersectionInternal(const math::Ray& ray, ObjRef& outRef, math::Intersection& outNearest, Material& outMaterial)
	{
		for (auto& plane : m_planes)
		{
			plane.isIntersecting(ray, outRef, outNearest, outMaterial);
		}
		for (auto& sphere : m_spheres)
		{
			sphere.isIntersecting(ray, outRef, outNearest, outMaterial);
		}

		for (auto& lightSphere : m_lightsVisualization)
		{
			lightSphere.isIntersecting(ray, outRef, outNearest, outMaterial);
		}

		for (auto& cube : m_cubes)
		{
			cube.isIntersecting(ray, outRef, outNearest, outMaterial);
		}
	}

	// without lights visualizations (their spheres)
	void Scene::findShadowRayIntersectionInternal(const math::Ray& ray, ObjRef& outRef, math::Intersection& outNearest, Material& outMaterial)
	{
		for (auto& plane : m_planes)
		{
			plane.isIntersecting(ray, outRef, outNearest, outMaterial);
		}

		for (auto& sphere : m_spheres)
		{
			sphere.isIntersecting(ray, outRef, outNearest, outMaterial);
		}

		for (auto& cube : m_cubes)
		{
			cube.isIntersecting(ray, outRef, outNearest, outMaterial);
		}
	}

	void Scene::lighting(math::Vec3f& outColor, const math::Vec3f& cameraPosition, const math::Vec3f& position, const math::Vec3f& normal, const Material& material)
	{
		if (material.isEmissive)
		{
			outColor = material.albedo;
			return;
		}

		math::Vec3f ambient = m_backgroundColor.cwiseProduct(material.albedo) * m_ambientFactor;
		math::Vec3f diffuseAndSpecular = math::Vec3f(0.0f, 0.0f, 0.0f);

		math::Ray shadowRay;
		shadowRay.origin = math::Vec3f(position[0] + 0.01f * normal[0], position[1] + 0.01f * normal[1], position[2] + 0.01f * normal[2]);
		math::Intersection shadowRayIntersection;
		Material mat;
		Scene::ObjRef ref = { nullptr, Scene::IntersectedType::NUM };

		// directional light
		for (auto& directionalLight : m_directionalLights)
		{
			shadowRay.direction = -directionalLight.getDirection();
			shadowRayIntersection.reset();

			findShadowRayIntersectionInternal(shadowRay, ref, shadowRayIntersection, mat);
			if (!shadowRayIntersection.valid())
			{
				diffuseAndSpecular += calculateLighting_DirectionalLight(directionalLight, cameraPosition, position, normal, material);
			}
		}

		// point lights
		for (auto& pointLight : m_pointLights)
		{
			math::Vec3f toLight = pointLight.getPos() - shadowRay.origin;
			shadowRay.direction = toLight.normalized();
			float maxT = toLight.norm();

			shadowRayIntersection.reset();

			findShadowRayIntersectionInternal(shadowRay, ref, shadowRayIntersection, mat);
			if (!shadowRayIntersection.valid() || shadowRayIntersection.t > maxT)
			{
				diffuseAndSpecular += calculateLighting_PointLight(pointLight, cameraPosition, position, normal, material);
			}
		}

		// spot lights
		for (auto& spotLight : m_spotLights)
		{
			if (spotLight.isInside(shadowRay.origin))
			{
				math::Vec3f toLight = spotLight.getPos() - shadowRay.origin;
				shadowRay.direction = toLight.normalized();
				float maxT = toLight.norm();

				shadowRayIntersection.reset();

				findShadowRayIntersectionInternal(shadowRay, ref, shadowRayIntersection, mat);
				if (!shadowRayIntersection.valid() || shadowRayIntersection.t > maxT)
				{
					diffuseAndSpecular += calculateLighting_SpotLight(spotLight, cameraPosition, position, normal, material);
				}
			}
		}
		math::Vec3f result = ambient + diffuseAndSpecular;

		//clamping to 0.0f - 1.0f
		outColor = result.cwiseMin(1.0f);
	}

}