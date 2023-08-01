#pragma once
#include "../utils/nonCopyable.h"
#include "../render/texture/texture.h"
#include <unordered_map>
#include <memory>

namespace Engine
{
	class TextureManager
		: public NonCopyable
	{
	public:
		static TextureManager* createInstance();
		static void deleteInstance();
		static TextureManager* getInstance();

		void init();
		void deinit();

		std::shared_ptr<Texture> getTexture(const std::wstring& filePath);
		std::shared_ptr<Texture> getPerlinNoise3DTexture();
	private:
		TextureManager() = default;
		static TextureManager* s_instance;

		void generatePerlinNoise3DTexture();

		std::unordered_map<std::wstring, std::shared_ptr<Texture>> m_textures;
		const std::wstring PERLIN_NOISE_3D_TEXTURE_NAME = L"PerlinNoise3D";

		std::shared_ptr<Texture> loadTexture(const std::wstring& filePath);
		void deleteAllTextures();
	};
}