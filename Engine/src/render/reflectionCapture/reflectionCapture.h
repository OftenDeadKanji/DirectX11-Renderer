#pragma once
#include "../shader/shader.h"
#include "../texture/texture.h"
#include "../Direct3d/buffer.h"

namespace Engine
{
	class ReflectionCapture
	{
	public:
		ReflectionCapture();
		~ReflectionCapture();

		void setEnvironmentTexture(std::shared_ptr<Texture> environmentTexture);

		void captureDiffuseReflection(std::wstring textureFileName);
		void captureSpecularReflection(std::wstring specularIrradianceTextureFileName, std::wstring specularFactorTextureFileName);
	private:
		std::shared_ptr<Texture> m_environmentTexture;

		Shader m_diffuseReflectionShader;
		Shader m_specularReflectionShader;
		Shader m_reflectanceFactorShader;

		struct EnvironmentTextureInfo
		{
			math::Vec2i size;
			math::Vec2i pad;
		};
		Buffer<EnvironmentTextureInfo> m_environmentTextureInfoCBuffer;

		void captureSpecularIrradianceReflection(std::wstring specularIrradianceTextureFileName);
		void captureSpecularFactorReflection(std::wstring specularFactorTextureFileName);
	};
}
