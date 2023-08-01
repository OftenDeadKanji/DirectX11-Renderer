#include "window.h"
#include "../utils/assert.h"
#include "../engine/renderer.h"

namespace Engine
{
	Window::Window(HWND windowHandle, float pixelBufferSizeMultiplier)
		: m_windowHandle(windowHandle), m_pixelBufferSizeMultiplier(pixelBufferSizeMultiplier)
	{
		RECT rect;
		GetClientRect(m_windowHandle, &rect);

		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		m_size = math::Vec2i(width, height);

		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.AlphaMode = DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.BufferCount = 2;
		desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Flags = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Scaling = DXGI_SCALING_NONE;
		desc.Stereo = false;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		HRESULT result = D3D::getInstancePtr()->getFactory()->CreateSwapChainForHwnd(D3D::getInstancePtr()->getDevice(), m_windowHandle, &desc, NULL, NULL, m_swapChain.reset());
		ALWAYS_ASSERT(result >= 0 && "CreateSwapChainForHwnd");

		result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_backbufferTex.reset());
		ALWAYS_ASSERT(result >= 0);
	}

	void Window::setPixelColor(const math::Vec2i& pixelPosition, const math::Vec3f& colorNormalized)
	{
		int index = pixelPosition.y() * m_pixelBuffer2DSize.x() + pixelPosition.x();

		m_pixelBuffer[index * 4 + 2] = colorNormalized.x() * 255;
		m_pixelBuffer[index * 4 + 1] = colorNormalized.y() * 255;
		m_pixelBuffer[index * 4 + 0] = colorNormalized.z() * 255;
	}

	math::Vec2i Window::getPixelBufferSize() const
	{
		return m_pixelBuffer2DSize;
	}

	void Window::setSize(const math::Vec2i& size)
	{
		m_size = size;

		m_pixelBuffer2DSize = (m_size.cast<float>() * m_pixelBufferSizeMultiplier).cast<int>();
		m_pixelBuffer.resize(m_pixelBuffer2DSize.x() * m_pixelBuffer2DSize.y() * 4);

		Renderer::getInstancePtr()->releaseBackbufferResources();
		m_backbufferTex.release();

		HRESULT hr = m_swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

		auto result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_backbufferTex.reset());
		ALWAYS_ASSERT(result >= 0);

		Renderer::getInstancePtr()->updateBackbuffer(m_backbufferTex);
		Renderer::getInstancePtr()->updateGBuffer();
	}

	math::Vec2i Window::getSize() const
	{
		return m_size;
	}

	void Window::setWindowTitle(const std::string& title)
	{
		SetWindowTextA(m_windowHandle, title.c_str());
	}
	void Window::flush()
	{
		m_swapChain->Present(0, 0);
	}
	DxResPtr<ID3D11Texture2D> Window::getBackbufferTex()
	{
		return m_backbufferTex;
	}
}