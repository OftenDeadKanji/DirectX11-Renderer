#pragma once
#include "../dependencies/Windows/win.h"
#include "../math/mathUtils.h"
#include <vector>
#include <cstdint>
#include "../render/Direct3d/DxRes.h"
#include "../render/Direct3d/d3d.h"

namespace Engine
{

	class Window
	{
	public:
		Window(HWND windowHandle, float pixelBufferSizeMultiplier);
		

		void setPixelColor(const math::Vec2i& pixelPosition, const math::Vec3f& colorNormalized);
		math::Vec2i getPixelBufferSize() const;

		void setSize(const math::Vec2i& size);
		math::Vec2i getSize() const;

		void setWindowTitle(const std::string& title);

		void flush();

		DxResPtr<ID3D11Texture2D> getBackbufferTex();
	private:

		math::Vec2i m_size;

		float m_pixelBufferSizeMultiplier;
		std::vector<uint8_t> m_pixelBuffer;
		math::Vec2i m_pixelBuffer2DSize;

		HWND m_windowHandle;
		DxResPtr<IDXGISwapChain1> m_swapChain;
		DxResPtr<ID3D11Texture2D> m_backbufferTex;
	};
}
