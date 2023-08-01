#pragma once
#include "../lights/directionalLight.h"
#include "../lights/pointLight.h"
#include "../lights/spotLight.h"
#include "../../utils/parallelExecutor.h"
#include "../material/material.h"
#include "transform.h"
#include "../../math/intersection.h"
#include "../../math/sphere.h"
#include "../../math/plane.h"
#include "../Direct3d/d3d.h"
#include "../shader/shader.h"
#include "../meshSystem/mesh/mesh.h"

namespace Engine
{
	class Window;
	class Camera;

	struct math::Ray;
	struct math::Intersection;

	class IObjectMover;
	class SphereMover;
	class MeshInstanceMover;
	class PlaneMover;
	class LightVisualizerMover;

	class Scene
	{
	protected:
		//to let those classes know about scene objects classes (Sphere, Plane, MeshInstance etc.)
		friend class SphereMover;
		friend class MeshInstanceMover;
		friend class PlaneMover;
		friend class LightVisualizerMover;

		friend class SphereRotator;


		enum class IntersectedType
		{
			Plane,
			Sphere,
			Mesh,
			LightVisualizer,
			NUM
		};

		struct ObjRef
		{
			void* object;
			IntersectedType type;
		};

		struct Sphere
			: public math::Sphere
		{
			Material material;

			bool isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial);
		};

		struct LightVisualizer
			: public Sphere
		{
			enum class LightType
			{
				Point,
				Spot,
				NUM
			};

			Light* light;
			LightType lightType;

			bool isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial);
		};

		struct Plane
			: public math::Plane
		{
			Material material;

			bool isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial);
		};

		struct MeshInstance
		{
			MeshInstance(const Mesh* meshPtr)
				: mesh(meshPtr)
			{}

			const Mesh* mesh;
			Material material;

			Transform transform;

			bool isIntersecting(const math::Ray& ray, Scene::ObjRef& ref, math::Intersection& outNearest, Material& outMaterial);
		};

	public:
		struct IntersectionQuery
		{
			math::Intersection nearest;
			Material* material;

			std::unique_ptr<IObjectMover>* mover;
		};

		Scene();

		void addPlane(const math::Vec3f& pointOnPlane, const math::Vec3f& normal, const Material& material);
		void addSphere(const math::Vec3f& position, float radius, const Material& material);
		void addCube(const Transform& position, const Material& material);

		void addDirectionalLight(const math::Vec3f& color, const math::Vec3f& direction);
		void addPointLight(const math::Vec3f& color, const math::Vec3f& position);
		void addSpotLight(const math::Vec3f& color, const math::Vec3f& position, const math::Vec3f& direction, float angle);

		void setAmbientLight(const math::Vec3f& color, float intensity);

		void render(Window&, Camera&);

		bool findIntersection(const math::Ray& ray, math::Intersection& outNearest, Material& outMaterial);
		bool findIntersection(const math::Ray& ray, IntersectionQuery& query);

	private:
		void computePixelColor(const math::Vec2i& pixelCoordiante, Window& window, Camera& camera);
		void findIntersectionInternal(const math::Ray& ray, ObjRef& outRef, math::Intersection& outNearest, Material& outMaterial);
		void findShadowRayIntersectionInternal(const math::Ray& ray, ObjRef& outRef, math::Intersection& outNearest, Material& outMaterial);
		void lighting(math::Vec3f& outColor, const math::Vec3f& cameraPosition, const math::Vec3f& position, const math::Vec3f& normal, const Material& material);

		std::vector<Plane> m_planes;
		std::vector<Sphere> m_spheres;
		std::vector<MeshInstance> m_cubes;

		math::Vec3f m_backgroundColor;
		float m_ambientFactor;

		std::vector<DirectionalLight> m_directionalLights;
		std::vector<PointLight> m_pointLights;
		std::vector<SpotLight> m_spotLights;

		std::vector<LightVisualizer> m_lightsVisualization;

		//MeshManager m_meshManager;

		ParallelExecutor m_parallelExecutor;
		
		DxResPtr<ID3D11Buffer> vertexBuffer;
		DxResPtr<ID3D11Buffer> indexBuffer;

		Shader shader;

		std::chrono::high_resolution_clock::time_point start;
	};
}