#pragma once
#include "../shader/shader.h"
#include "../Direct3d/buffer.h"
#include <vector>

#define MAX_VOLUMETRIC_FOG_INSTANCES 16

namespace Engine
{
	class Camera;

	class FogRenderer
	{
	public:
		FogRenderer();

		void update(const Camera& camera);

		void renderUniformFog(DxResPtr<ID3D11Texture2D> sceneTexture);
		void renderVolumetricFogs(Camera& camera, DxResPtr<ID3D11Texture2D> sceneTexture);

		void addVolumetricFog(const math::Vec3f& position, const math::Vec3f& scale);

		void setDensity(float density);
		float getDensity();
		void setPhaseFunctionParameter(float phase);
		float getPhaseFunctionParameter();

		void enableUniformFog();
		bool isUniformFogEnabled() const;

		void enableVolumetricFog();
		bool isVolumetricFogEnabled() const;

		void disableFog();
		bool isFogEnabled() const;

		const std::vector<math::Mat4f>& getVolumetricFogInstances() const;

		void bindFogInfoForVS(int slot);
		void bindFogInfoForPS(int slot);
		void bindVolumetricFogTextureForPS(int slot);
	private:
		float m_density;
		float m_phaseFunctionParameter;

		bool m_uniformFogEnabled = false;
		bool m_volumetricFogEnabled = false;

		Shader m_uniformFogShader;

		std::vector<math::Mat4f> m_volumetricFogInstances;
		Shader m_volumetricFogShader;
		
		struct FogInfo
		{
			uint32_t uniformFogEnabled;
			uint32_t volumetricFogEnabled;
			float fogDensity;
			float phaseFunctionParameter;

			unsigned int volumetricFogInstancesCount;
			uint32_t fogPad0;
			uint32_t fogPad1;
			uint32_t fogPad2;

			struct VolumetricFogInstance
			{
				math::Mat4f instance;
				math::Mat4f instanceInv;
			} volumetricFogInstances[MAX_VOLUMETRIC_FOG_INSTANCES];
		};
		Buffer<FogInfo> m_fogInfo;
	};
}