#include "lightSystem.h"
#include "../meshSystem/meshSystem.h"
#include "../../resourcesManagers/modelManager.h"
#include "../../engine/renderer.h"

namespace Engine
{
	LightSystem* LightSystem::s_instance = nullptr;

	LightSystem::LightSystem()
	{
		m_ambientLightEnergy = math::Vec3f(0.05f, 0.05f, 0.05f);

		m_directionalLight.energy = math::Vec3f(1.0f, 1.0f, 1.0f);
		m_directionalLight.direction = math::Vec3f(0.0f, -1.0f, 0.0f);

		m_lightsCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	}

	void LightSystem::updateDirectionalLightDepthCamera(const Camera& mainCamera, const Camera& prevCamera, bool adjustDepthCameraMovementToTexelSize)
	{
		auto& light = m_directionalLight;
		math::Vec4f mainCameraFrustumCorners[8] = {
				{ 1.0f,  1.0f, 1.0f, 1.0f},
				{-1.0f,  1.0f, 1.0f, 1.0f},
				{ 1.0f, -1.0f, 1.0f, 1.0f},
				{-1.0f, -1.0f, 1.0f, 1.0f},

				{ 1.0f,  1.0f, 0.0f, 1.0f},
				{-1.0f,  1.0f, 0.0f, 1.0f},
				{ 1.0f, -1.0f, 0.0f, 1.0f},
				{-1.0f, -1.0f, 0.0f, 1.0f}
		};

		float minDot = std::numeric_limits<float>::max(), maxDot = std::numeric_limits<float>::lowest();
		math::Vec3f nearest, farthest, middle(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 8; i++)
		{
			mainCamera.transformFromClipToWorldSpace(mainCameraFrustumCorners[i]);

			float dot = light.direction.dot(mainCameraFrustumCorners[i].head<3>() - mainCamera.position());
			if (dot < minDot)
			{
				minDot = dot;
				nearest = mainCameraFrustumCorners[i].head<3>();
			}
			if (dot > maxDot)
			{
				maxDot = dot;
				farthest = mainCameraFrustumCorners[i].head<3>();
			}

			middle += mainCameraFrustumCorners[i].head<3>();
		}

		middle /= 8.0f;

		const float margin = 5.0f;
		float zNear = minDot - margin;
		float zFar = maxDot;

		float zRange = std::abs(zFar - zNear);

		float H = (mainCameraFrustumCorners[0].head<3>() - mainCameraFrustumCorners[4].head<3>()).norm();

		float XY = 0.5f * H;

		math::Vec3f depthCameraNewPosition;
		if (!adjustDepthCameraMovementToTexelSize)
		{
			depthCameraNewPosition = middle - light.direction * zRange * 0.5f;
		}
		else
		{
			float textureSize = static_cast<float>(Renderer::getInstancePtr()->getDirectionalLightShadowResolution());

			math::Vec2f uv1 = { 0.5f / textureSize, 0.5f / textureSize };
			math::Vec2f uv2 = { 1.5f / textureSize, 0.5f / textureSize };

			uv1 = (uv1.array() - 0.5f) * 2.0f;
			uv2 = (uv2.array() - 0.5f) * 2.0f;

			math::Vec4f xyz1 = { uv1.x(), uv1.y(), 0.0, 1.0 };
			math::Vec4f xyz2 = { uv2.x(), uv2.y(), 0.0, 1.0 };

			xyz1 *= m_directionalLight.depthCamera.getViewProjInv();
			xyz1 /= xyz1.w();

			xyz2 *= m_directionalLight.depthCamera.getViewProjInv();
			xyz2 /= xyz2.w();

			float texelSize = abs(xyz1.x() - xyz2.x());

			math::Vec3f newPos = middle - light.direction * zRange * 0.5f;
			math::Vec4f newPos4 = { newPos.x(), newPos.y(), newPos.z(), 1.0f };
			m_directionalLight.depthCamera.transformFromWorldToViewSpace(newPos4);

			math::Vec3f prevPosition = m_directionalLight.depthCamera.position();
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
			prevPosition += prevCamera.position();
#endif
			math::Vec4f prevPosition4 = { prevPosition.x(), prevPosition.y(), prevPosition.z(), 1.0f };
			m_directionalLight.depthCamera.transformFromWorldToViewSpace(prevPosition4);

			math::Vec3f posDiff = newPos4.head<3>() - prevPosition4.head<3>();

			float xDiff = posDiff.x() / texelSize;
			float xWhole;
			std::modf(xDiff, &xWhole);

			float yDiff = posDiff.y() / texelSize;
			float yWhole;
			std::modf(yDiff, &yWhole);

			posDiff = math::Vec3f(xWhole * texelSize, yWhole * texelSize, posDiff.z());
			math::Vec4f posDiff4 = { posDiff.x(), posDiff.y(), posDiff.z(), 0.0f };
			m_directionalLight.depthCamera.transformFromViewtoWorldSpace(posDiff4);
			
			depthCameraNewPosition = prevPosition + posDiff4.head<3>();
		}

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
		depthCameraNewPosition -= mainCamera.position();
#endif

		m_directionalLight.depthCamera.lookAt(depthCameraNewPosition, depthCameraNewPosition + m_directionalLight.direction);
		m_directionalLight.depthCamera.setOrthographic(XY, -XY, XY, -XY, 0.0f, zRange);
		m_directionalLight.depthCamera.updateCamera();
	}

	void LightSystem::initPointLightDepthCameras(PointLight& light)
	{
		math::Angles cameraAngles[6] = {
			math::Angles({0.0f, math::deg2rad(-90.0f), 0.0f}),
			math::Angles({0.0f, math::deg2rad(90.0f), 0.0f}),
			math::Angles({math::deg2rad(90.0f), 0.0f, 0.0f}),
			math::Angles({math::deg2rad(-90.0f), 0.0f, 0.0f}),
			math::Angles({0.0f, 0.0f ,0.0f}),
			math::Angles({0.0f, math::deg2rad(180.0f), 0.0f})
		};
		
		for (int i = 0; i < 6; i++)
		{
			light.depthCamera[i].setWorldAngles(cameraAngles[i]);
			light.depthCamera[i].setPerspective(90.0f, 1.0f, 0.1f, 100.0f);
			light.depthCamera[i].updateMatrices();
		}
	}

	void LightSystem::init()
	{
	}
	void LightSystem::deinit()
	{
	}
	void LightSystem::update(const Camera& mainCamera, const Camera& prevCamera )
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();

		//dir light
		{
			updateDirectionalLightDepthCamera(mainCamera, prevCamera, true);
		}
		
		auto* trSys = TransformSystem::getInstance();
		//point lights
		{
			for (auto& light : m_pointLights)
			{
				for (auto& camera : light.depthCamera)
				{
					math::Vec3f position = light.position + math::getTranslation(trSys->getMatrix(light.transformMatrixID));

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
					position -= mainCamera.position();
#endif

					camera.setWorldOffset(position);

					camera.updateMatrices();
				}
			}
		}
		
		//spot light
		{
			math::Vec3f pos = math::getTranslation(m_spotLight.transformMatrix);

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
			pos -= mainCamera.position();
#endif

			math::Vec3f dir = math::getForward(m_spotLight.transformMatrix);
			m_spotLight.depthCamera.lookAt(pos, pos + dir);
			m_spotLight.depthCamera.updateMatrices();
		}
		
		auto& res = m_lightsCBuffer.map(devcon);
		LightsCBuffer* ptr = static_cast<LightsCBuffer*>(res.pData);
		*ptr = LightsCBuffer(this);
		
#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
		math::Vec3f camPos = mainCamera.position();

		for (int i = 0; i < ptr->pointLightsCount; i++)
		{
			ptr->pointLights[i].position -= camPos;
		}

		ptr->spotLightPosition -= camPos;

		math::Mat4f camPosMat = math::Mat4f::Identity();
		math::setTranslation(camPosMat, camPos);

		ptr->spotLightProjectorMatrix = camPosMat * ptr->spotLightProjectorMatrix;
#endif
		m_lightsCBuffer.unmap(devcon);
	}

	void LightSystem::setAmbientLight(const math::Vec3f& energy)
	{
		m_ambientLightEnergy = energy;
	}
	void LightSystem::setDirectionalLight(const math::Vec3f& energy, const math::Vec3f& direction, float solidAngle, float perceivedRadius, const Camera& mainCamera)
	{
		m_directionalLight.energy = energy;
		m_directionalLight.direction = direction;
		m_directionalLight.solidAngle = solidAngle;
		m_directionalLight.perveivedRadius = perceivedRadius;
		m_directionalLight.perveivedDistance = perceivedRadius / sqrt(1 - pow(1 - solidAngle / (2 * math::PI), 2));

		updateDirectionalLightDepthCamera(mainCamera, mainCamera, false);
	}

	LightSystem::DirectionalLight& LightSystem::getDirectionalLight()
	{
		return m_directionalLight;
	}

	void LightSystem::addPointLight(const math::Vec3f& position, const math::Vec3f& energy, TransformSystem::ID transformMatrixID, float radius)
	{
		if (m_pointLights.size() >= MAX_POINT_LIGHTS)
		{
			_DEBUG_OUTPUT("There is max number of point lights in scene. This light won't affect the final render.")
		}
		m_pointLights.emplace_back();
		m_pointLights.back().position = position;
		m_pointLights.back().energy = energy;
		m_pointLights.back().transformMatrixID = transformMatrixID;
		m_pointLights.back().radius = radius;

		initPointLightDepthCameras(m_pointLights.back());
	}

	std::vector<LightSystem::PointLight>& LightSystem::getPointLights()
	{
		return m_pointLights;
	}

	void LightSystem::setSpotLight(const math::Vec3f& energy, const math::Mat4f& transform, float angle, std::shared_ptr<Texture> maskTexture, float radius)
	{
		m_spotLight.energy = energy;
		m_spotLight.transformMatrix = transform;
		m_spotLight.angle = angle;
		m_spotLight.maskTexture = maskTexture;
		m_spotLight.radius = radius;

		m_spotLight.depthCamera.setPerspective(90.0f, 1.0f, 0.1f, 100.0f);
	}

	void LightSystem::setSpotLightTransform(const math::Mat4f& transform)
	{
		m_spotLight.transformMatrix = transform;
	}

	LightSystem::SpotLight& LightSystem::getSpotLight()
	{
		return m_spotLight;
	}

	void LightSystem::setPerFrameBufferForVS(ID3D11DeviceContext4* devcon)
	{
		m_lightsCBuffer.setConstantBufferForVertexShader(devcon, 2);
		m_spotLight.maskTexture->bindSRVForVS(0);
	}

	void LightSystem::setPerFrameBufferForGS(ID3D11DeviceContext4* devcon)
	{
		m_lightsCBuffer.setConstantBufferForGeometryShader(devcon, 2);
		m_spotLight.maskTexture->bindSRVForGS(0);
	}

	void LightSystem::setPerFrameBufferForPS(ID3D11DeviceContext4* devcon)
	{
		m_lightsCBuffer.setConstantBufferForPixelShader(devcon, 2);
		m_spotLight.maskTexture->bindSRVForPS(0);
	}

	LightSystem::LightsCBuffer::LightsCBuffer(const LightSystem* instance)
	{
		//ambient light
		ambientLightEnergy = instance->m_ambientLightEnergy;

		//directional light
		directionalLightEnergy = instance->m_directionalLight.energy;
		directionalLightDirection = instance->m_directionalLight.direction;
		directionalLightSolidAngle = instance->m_directionalLight.solidAngle;
		directionalLightPerceivedRadius = instance->m_directionalLight.perveivedRadius;
		directionalLightPerceivedDistance = instance->m_directionalLight.perveivedDistance;
		directionalLightDepthViewProj = instance->m_directionalLight.depthCamera.getViewProj();
		directionalLightDepthViewProjInv = instance->m_directionalLight.depthCamera.getViewProjInv();

		//point lights
		pointLightsCount = std::min(instance->m_pointLights.size(), static_cast<size_t>(MAX_POINT_LIGHTS));

		for (int i = 0; i < pointLightsCount; i++)
		{
			for (int j = 0; j < 6; j++)
			{
				this->pointLights[i].depthViewProj[j] = instance->m_pointLights[i].depthCamera[j].getViewProj();
				this->pointLights[i].depthViewProjInv[j] = instance->m_pointLights[i].depthCamera[j].getViewProjInv();
			}
			this->pointLights[i].energy = instance->m_pointLights[i].energy;
			const math::Mat4f& transformMatrix = TransformSystem::getInstance()->getMatrix(instance->m_pointLights[i].transformMatrixID);
			this->pointLights[i].position = (math::Vec4f(instance->m_pointLights[i].position.x(), instance->m_pointLights[i].position.y(), instance->m_pointLights[i].position.z(), 1.0f) * transformMatrix).head<3>();
			this->pointLights[i].radius = instance->m_pointLights[i].radius;
			this->pointLights[i].cameraZNear = instance->m_pointLights[i].depthCamera[0].getZNear();
			this->pointLights[i].cameraZFar = instance->m_pointLights[i].depthCamera[0].getZFar();
		}

		//spot light
		spotLightEnergy = instance->m_spotLight.energy;
		spotLightPosition = math::getTranslation(instance->m_spotLight.transformMatrix);
		spotLightDirection = math::getForward(instance->m_spotLight.transformMatrix);
		spotLightCosAngle = cos(math::deg2rad(instance->m_spotLight.angle));
		spotLightRadius = instance->m_spotLight.radius;


		math::Mat4f spotLightViewMatrix;
		math::invertOrthonormal(instance->m_spotLight.transformMatrix, spotLightViewMatrix);

		math::Mat4f spotLightProjMatrix = math::createPerspectiveProjectionMatrix(instance->m_spotLight.angle * 2.0f, 1.0f, 0.1f, 1000.0f);

		math::Mat4f bias = math::Mat4f::Identity();
		math::setTranslation(bias, math::Vec3f(0.5f, 0.5f, 0.5f));
		math::setScale(bias, math::Vec3f(0.5f, 0.5f, 0.5f));

		spotLightProjectorMatrix = spotLightViewMatrix * spotLightProjMatrix * bias;
		spotLightDepthViewProj = instance->m_spotLight.depthCamera.getViewProj();
		spotLightDepthViewProjInv = instance->m_spotLight.depthCamera.getViewProjInv();
	}
}