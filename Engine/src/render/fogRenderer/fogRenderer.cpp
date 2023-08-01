#include "fogRenderer.h"
#include "../../engine/renderer.h"
#include "../camera/camera.h"
#include "../../resourcesManagers/textureManager.h"

namespace Engine
{
	FogRenderer::FogRenderer()
	{
		m_uniformFogShader.init(L"Shaders/fog/fogVS.hlsl", L"Shaders/fog/fogPS.hlsl", {});
		m_volumetricFogShader.init(L"Shaders/fog/volumetricFogVS.hlsl", L"Shaders/fog/volumetricFogPS.hlsl", {});

		m_density = 0.1;
		m_phaseFunctionParameter = -0.7;

		this->addVolumetricFog({ 0.0f, 3.0f, 0.0f }, {3.0f, 3.0f, 3.0f});

		m_fogInfo.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	}

	void FogRenderer::update(const Camera& camera)
	{
		auto res = m_fogInfo.map(D3D::getInstancePtr()->getDeviceContext());
		FogInfo* info = static_cast<FogInfo*>(res.pData);

		info->uniformFogEnabled = static_cast<uint32_t>(m_uniformFogEnabled);
		info->volumetricFogEnabled = static_cast<uint32_t>(m_volumetricFogEnabled);
		info->fogDensity = m_density;
		info->phaseFunctionParameter = m_phaseFunctionParameter;

		info->volumetricFogInstancesCount = m_volumetricFogInstances.size();

		for (int i = 0; i < m_volumetricFogInstances.size(); i++)
		{
			math::Mat4f tr = m_volumetricFogInstances[i];

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
			math::Vec3f pos = math::getTranslation(tr);
			math::setTranslation(tr, pos - camera.position());
#endif

			info->volumetricFogInstances[i].instance = tr;
			info->volumetricFogInstances[i].instanceInv = tr.inverse();
		}

		m_fogInfo.unmap(D3D::getInstancePtr()->getDeviceContext());
	}

	void FogRenderer::renderUniformFog(DxResPtr<ID3D11Texture2D> sceneTexture)
	{
		auto* renderer = Renderer::getInstancePtr();

		m_uniformFogShader.bind();
		renderer->setPerFrameBuffersForPS();
		renderer->setPerFrameGlobalSamplersForPS();

		auto gbuffer = renderer->getGBuffer();

		DxResPtr<ID3D11ShaderResourceView> sceneSRV;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(sceneTexture, nullptr, sceneSRV.reset());

		ID3D11ShaderResourceView* SRVs[] =
		{
			sceneSRV.ptr(),
			gbuffer.depthSRV.ptr()
		};

		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 2, SRVs);

		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
	}

	void FogRenderer::renderVolumetricFogs(Camera& camera, DxResPtr<ID3D11Texture2D> sceneTexture)
	{
		if (m_volumetricFogInstances.empty())
		{
			return;
		}

		auto* renderer = Renderer::getInstancePtr();

		m_volumetricFogShader.bind();

		renderer->setPerFrameBuffersForVS();
		renderer->setPerFrameBuffersForPS();

		renderer->setPerViewBuffersForVS();
		renderer->setPerViewBuffersForPS();

		renderer->setPerFrameGlobalSamplersForPS();
		
		auto gbuffer = Renderer::getInstancePtr()->getGBuffer();

		DxResPtr<ID3D11ShaderResourceView> sceneSRV;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(sceneTexture, nullptr, sceneSRV.reset());

		ID3D11ShaderResourceView* SRVs[] =
		{
			sceneSRV.ptr(),
			gbuffer.depthSRV.ptr()
		};

		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 2, SRVs);

		auto fogTex = TextureManager::getInstance()->getPerlinNoise3DTexture();
		fogTex->bindSRVForPS(22);

		unsigned int indices[] = {
			0, 3, 1,
			0, 2, 3,

			1, 7, 5,
			1, 3, 7,

			4, 2, 0,
			4, 6, 2,

			5, 6, 4,
			5, 7, 6,

			4, 1, 5,
			4, 0, 1,

			2, 7, 3,
			2, 6, 7
		};

		Buffer<unsigned int> indicesBuffer;
		indicesBuffer.createIndexBuffer(36, indices, D3D::getInstancePtr()->getDevice());

		indicesBuffer.setIndexBufferForInputAssembler(D3D::getInstancePtr()->getDeviceContext());
		D3D::getInstancePtr()->getDeviceContext()->DrawIndexedInstanced(36, m_volumetricFogInstances.size(), 0, 0, 0);
	}

	void FogRenderer::addVolumetricFog(const math::Vec3f& position, const math::Vec3f& scale)
	{
		math::Mat4f transform = math::Mat4f::Identity();
		
		math::setScale(transform, scale);
		math::setTranslation(transform, position);

		m_volumetricFogInstances.push_back(transform);
	}

	void FogRenderer::setDensity(float density)
	{
		m_density = density;
	}

	float FogRenderer::getDensity()
	{
		return m_density;
	}

	void FogRenderer::setPhaseFunctionParameter(float phase)
	{
		m_phaseFunctionParameter = phase;
	}

	float FogRenderer::getPhaseFunctionParameter()
	{
		return m_phaseFunctionParameter;
	}

	void FogRenderer::enableUniformFog()
	{
		m_uniformFogEnabled = true;
		m_volumetricFogEnabled = false;
	}

	bool FogRenderer::isUniformFogEnabled() const
	{
		return m_uniformFogEnabled;
	}

	void FogRenderer::enableVolumetricFog()
	{
		m_uniformFogEnabled = false;
		m_volumetricFogEnabled = true;
	}

	bool FogRenderer::isVolumetricFogEnabled() const
	{
		return m_volumetricFogEnabled;
	}

	void FogRenderer::disableFog()
	{
		m_uniformFogEnabled = false;
		m_volumetricFogEnabled = false;
	}

	bool FogRenderer::isFogEnabled() const
	{
		return m_uniformFogEnabled || m_volumetricFogEnabled;
	}

	const std::vector<math::Mat4f>& FogRenderer::getVolumetricFogInstances() const
	{
		return m_volumetricFogInstances;
	}

	void FogRenderer::bindFogInfoForVS(int slot)
	{
		m_fogInfo.setConstantBufferForVertexShader(D3D::getInstancePtr()->getDeviceContext(), slot);
	}
	void FogRenderer::bindFogInfoForPS(int slot)
	{
		m_fogInfo.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), slot);
	}
	void FogRenderer::bindVolumetricFogTextureForPS(int slot)
	{
		auto fogTex = TextureManager::getInstance()->getPerlinNoise3DTexture();
		fogTex->bindSRVForPS(slot);
	}
}