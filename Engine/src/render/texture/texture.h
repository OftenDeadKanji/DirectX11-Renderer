#pragma once
#include "../Direct3d/d3d.h"
#include <string>
#include "../../math/mathUtils.h"

namespace Engine
{
	class TextureManager;

	class Texture
	{
		friend class TextureManager;

	public:
		Texture() = default;

		void bindSRVForVS(int slot = 0);
		void bindSRVForGS(int slot = 0);
		void bindSRVForPS(int slot = 0);

		math::Vec2i getSize() const;
		unsigned int getMipLevels() const;
	private:
		DxResPtr<ID3D11Texture2D> m_texture2D;
		DxResPtr<ID3D11Texture3D> m_texture3D;
		DxResPtr<ID3D11ShaderResourceView> m_shaderResourceView;

		math::Vec2i m_size;
		unsigned int m_mipLevels;
	};
}
