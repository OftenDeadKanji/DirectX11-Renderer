#include "skyRenderer.h"
#include "../camera/camera.h"
#include "../../engine/renderer.h"

namespace Engine
{
	void SkyRenderer::init(std::shared_ptr<Texture> texture)
	{
		m_texture = texture;

		shader.init(L"Shaders/sky/skyVS.hlsl", L"Shaders/sky/skyPS.hlsl", {});
	}

	std::shared_ptr<Texture> SkyRenderer::getSkyTexture()
	{
		return m_texture;
	}

	void SkyRenderer::render(Camera& camera)
	{
		if (m_texture)
		{
			shader.bind();

			Renderer::getInstancePtr()->setPerFrameBuffersForVS();

			m_texture->bindSRVForPS(20);
			Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

			D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
		}
	}
}