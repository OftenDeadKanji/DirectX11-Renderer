#pragma once
#include "../texture/texture.h"
#include "../shader/shader.h"

namespace Engine
{
	class Camera;

	class SkyRenderer
	{
	public:
		SkyRenderer() = default;

		void init(std::shared_ptr<Texture> texture);

		std::shared_ptr<Texture> getSkyTexture();

		void render(Camera& camera);
	private:
		std::shared_ptr<Texture> m_texture;
		Shader shader;
	};
}