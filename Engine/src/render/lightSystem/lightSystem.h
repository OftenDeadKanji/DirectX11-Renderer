#pragma once
#include "../../utils/nonCopyable.h"
#include <vector>
#include "../../math/mathUtils.h"
#include "../Direct3d/buffer.h"
#include "../../transformSystem/transformSystem.h"
#include "../camera/camera.h"

#define MAX_POINT_LIGHTS 32

namespace Engine
{
	class Texture;

	class LightSystem
		: public NonCopyable
	{
	public:
		struct DirectionalLight
		{
			Camera depthCamera;
			math::Vec3f energy;
			math::Vec3f direction;
			float solidAngle;
			float perveivedRadius;
			float perveivedDistance;
		};
		struct PointLight
		{
			Camera depthCamera[6];
			math::Vec3f position;
			math::Vec3f energy;
			TransformSystem::ID transformMatrixID;
			float radius;
		};
		struct SpotLight
		{
			Camera depthCamera;
			math::Vec3f energy;
			math::Mat4f transformMatrix;
			float angle;
			float radius;

			std::shared_ptr<Texture> maskTexture;
		};
	public:
		static LightSystem* createInstance()
		{
			if (!s_instance)
			{
				s_instance = new LightSystem();
			}

			return s_instance;
		}

		static LightSystem* getInstancePtr()
		{
			return s_instance;
		}

		static void deleteInstance()
		{
			s_instance->deinit();
			delete s_instance;

			s_instance = nullptr;
		}

		void init();
		void deinit();

		void update(const Camera& camera, const Camera& prevCamera);

		void setAmbientLight(const math::Vec3f& energy);

		void setDirectionalLight(const math::Vec3f& energy, const math::Vec3f& direction, float solidAngle, float perceivedRadius, const Camera& mainCamera);
		DirectionalLight& getDirectionalLight();

		void addPointLight(const math::Vec3f& position, const math::Vec3f& energy, TransformSystem::ID transformMatrixID, float radius);
		std::vector<PointLight>& getPointLights();

		void setSpotLight(const math::Vec3f& energy, const math::Mat4f& transform, float angle, std::shared_ptr<Texture> maskTexture, float radius);
		void setSpotLightTransform(const math::Mat4f& transform);
		SpotLight& getSpotLight();

		void setPerFrameBufferForVS(ID3D11DeviceContext4* devcon);
		void setPerFrameBufferForGS(ID3D11DeviceContext4* devcon);
		void setPerFrameBufferForPS(ID3D11DeviceContext4* devcon);
	private:
		LightSystem();
		static LightSystem* s_instance;

		void initPointLightDepthCameras(PointLight& light);

		void updateDirectionalLightDepthCamera(const Camera& mainCamera, const Camera& prevCamera, bool adjustDepthCameraMovementToTexelSize);

		math::Vec3f m_ambientLightEnergy;

		
		DirectionalLight m_directionalLight;

		
		std::vector<PointLight> m_pointLights;

		
		SpotLight m_spotLight;

		struct LightsCBuffer
		{
			math::Vec3f ambientLightEnergy;
			
			//point lights
			int pointLightsCount;

			struct
			{
				math::Mat4f depthViewProj[6];
				math::Mat4f depthViewProjInv[6];

				math::Vec3f energy;
				float radius;

				math::Vec3f position;
				float cameraZNear;

				float cameraZFar;
				float pad[3];
			} pointLights[MAX_POINT_LIGHTS];

			// directional light
			math::Mat4f directionalLightDepthViewProj;
			math::Mat4f directionalLightDepthViewProjInv;

			math::Vec3f directionalLightEnergy;
			float directionalLightSolidAngle;

			math::Vec3f directionalLightDirection;
			float directionalLightPerceivedRadius;

			float directionalLightPerceivedDistance;
			math::Vec3f pad0;

			// spot light
			math::Mat4f spotLightDepthViewProj;
			math::Mat4f spotLightDepthViewProjInv;
			math::Mat4f spotLightProjectorMatrix;

			math::Vec3f spotLightEnergy;
			float spotLightCosAngle;
			math::Vec3f spotLightPosition;
			float spotLightRadius;

			math::Vec3f spotLightDirection;
			float pad1;

			LightsCBuffer() = default;
			LightsCBuffer(const LightSystem* instance);
		};

		Buffer<LightsCBuffer> m_lightsCBuffer;
	};
}
