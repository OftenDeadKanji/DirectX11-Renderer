#include "renderer.h"
#include "../utils/assert.h"
#include "../render/meshSystem/meshSystem.h"
#include "../render/shader/shader.h"
#include "../render/lightSystem/lightSystem.h"
#include "../render/particleSystem/particleSystem.h"
#include "../render/decalSystem/decalSystem.h"

namespace Engine
{
	Renderer* Renderer::s_instance = nullptr;

	Renderer* Renderer::createInstance()
	{
		if (!s_instance)
		{
			s_instance =  new Renderer();
		}

		return s_instance;
	}
	void Renderer::deleteInstance()
	{
		s_instance->deinit();
		delete s_instance;

		s_instance = nullptr;
	}
	Renderer* Renderer::getInstancePtr()
	{
		return s_instance;
	}

	void Renderer::init(DxResPtr<ID3D11Texture2D> backbufferTexture)
	{
		m_clearColor = math::Vec3f(0.0f, 0.0f, 0.0f);
		
		createBackbuffer(backbufferTexture);

		perFrameConstantBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
		perViewConstantBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
		
		initSamplers();
		initDepthRendering();
		initBlendState();
		initGBuffer();

		postProcess.init();
		m_gamma = 2.2f;
		m_ev100 = 1.0f;

		fxaaQualitySubpix = 0.75f;
		fxaaQualityEdgeThreshold = 0.063f;
		fxaaQualityEdgeThresholdMin = 0.0312f;
	}

	void Renderer::deinit()
	{
		perFrameConstantBuffer.reset();
		perViewConstantBuffer.reset();
		m_depthStencilViewReadWrite.release();
		m_DSSDeptEnabledStencilDisabled.release();
		m_DSSDeptEnabledStencilReadWrite.release();
		m_DSSDeptEnabledStencilReadOnly.release();
		m_depthStencilBuffer.release();
		m_hdrRTV.release();
		m_ldrRTV.release();
	}

	void Renderer::update(const Camera& camera, const Camera& prevCamera, float deltaTime, float timeSinceStart)
	{
		updatePerFrameData(camera, prevCamera, deltaTime, timeSinceStart);
		ParticleSystem::getInstance()->updateCPUParticles(deltaTime);
		m_fogRenderer.update(camera);
	}

	void Renderer::render(Camera& camera)
	{
		clearViews();

		MeshSystem::getInstancePtr()->updateShadingGroupsInstanceBuffers(camera);

		renderDepth(camera);

		D3D::getInstancePtr()->getDeviceContext()->RSSetState(m_rasterizerState);

		renderGBufferGeometry(camera);
		renderDecals(camera);
		renderGBufferLighing(camera);

		ParticleSystem::getInstance()->updateGPUParticles();
		ParticleSystem::getInstance()->renderGPUParticlesLights();

		renderSkybox(camera);
		renderUniformFog();
		renderVolumetricFog(camera);
		
		renderParticleSystem(camera);

		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
		applyBloom();
		convertHDRtoLDR();
		applyAA();
	}

	void Renderer::renderMeshes(Camera& camera)
	{
		auto* hdrRTV = m_hdrRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &hdrRTV, m_depthStencilViewReadWrite);
		D3D::getInstancePtr()->getDeviceContext()->RSSetState(m_rasterizerState);

		updatePerViewData(camera);

		auto* srvDir = depthDirLightSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(4, 1, &srvDir);
		auto* srvPoint = depthCubemapSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(5, 1, &srvPoint);
		auto* srvSpot = depthSpotLightSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(6, 1, &srvSpot);
		
		//MeshSystem::getInstancePtr()->renderStencil();
		MeshSystem::getInstancePtr()->render();

		ID3D11ShaderResourceView* const nullSRV[1] = { NULL };
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(4, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(5, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(6, 1, nullSRV);
	}

	void Renderer::renderUniformFog()
	{
		if (m_fogRenderer.isUniformFogEnabled())
		{

			D3D11_TEXTURE2D_DESC desc;
			m_hdrTexture->GetDesc(&desc);

			DxResPtr<ID3D11Texture2D> hdrTexCopy, hdrCurrentTex;
			D3D::getInstancePtr()->getDevice()->CreateTexture2D(&desc, nullptr, hdrTexCopy.reset());
			m_hdrRTV->GetResource((ID3D11Resource**)hdrCurrentTex.reset());

			D3D::getInstancePtr()->getDeviceContext()->CopyResource(hdrTexCopy, hdrCurrentTex);

			auto rtv = m_hdrRTV.ptr();
			D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, nullptr);

			m_fogRenderer.renderUniformFog(hdrTexCopy);
		}
	}

	void Renderer::renderVolumetricFog(Camera& camera)
	{
		if (m_fogRenderer.isVolumetricFogEnabled())
		{

			D3D11_TEXTURE2D_DESC desc;
			m_hdrTexture->GetDesc(&desc);

			DxResPtr<ID3D11Texture2D> hdrTexCopy, hdrCurrentTex;
			D3D::getInstancePtr()->getDevice()->CreateTexture2D(&desc, nullptr, hdrTexCopy.reset());
			m_hdrRTV->GetResource((ID3D11Resource**)hdrCurrentTex.reset());

			D3D::getInstancePtr()->getDeviceContext()->CopyResource(hdrTexCopy, hdrCurrentTex);

			auto rtv = m_hdrRTV.ptr();
			D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, nullptr);

			this->setFrontFaceCulling();

			m_fogRenderer.renderVolumetricFogs(camera, hdrTexCopy);

			this->setBackFaceCulling();
		}
	}

	void Renderer::renderSkybox(Camera& camera)
	{
		auto hdrRTV = m_hdrRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &hdrRTV, m_GBuffer.m_depthStencilViewReadOnly);

		m_skyRenderer.render(camera);
	}

	void Renderer::renderParticleSystem(Camera& camera)
	{
		Renderer::getInstancePtr()->enableBlending();

		auto* depthSRV = m_GBuffer.depthSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(23, 1, &depthSRV);
		D3D::getInstancePtr()->getDeviceContext()->CSSetShaderResources(23, 1, &depthSRV);

		ParticleSystem::getInstance()->renderParticles(camera);

		ID3D11ShaderResourceView* const nullSRV[1] = { NULL };
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(23, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->CSSetShaderResources(23, 1, nullSRV);
	}

	void Renderer::convertHDRtoLDR()
	{
		auto ldrRTV = m_ldrRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &ldrRTV, nullptr);

		postProcess.setGamma(m_gamma);
		postProcess.setEV100(m_ev100);
		postProcess.resolve(m_hdrRTV, m_ldrRTV);
	}

	void Renderer::applyAA()
	{
		auto backbufferRTV = m_backbufferRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &backbufferRTV, nullptr);

		postProcess.applyFXAA(m_ldrRTV, m_backbufferRTV);
	}

	void Renderer::applyBloom()
	{
		postProcess.applyBloom(m_hdrRTV, m_hdrRTV, 5);
	}

	void Renderer::setPerFrameBuffersForVS()
	{
		perFrameConstantBuffer.setConstantBufferForVertexShader(D3D::getInstancePtr()->getDeviceContext(), 0);
		gbufferConstantBuffer.setConstantBufferForVertexShader(D3D::getInstancePtr()->getDeviceContext(), 3);
		m_fogRenderer.bindFogInfoForVS(4);
	}

	void Renderer::setPerFrameBuffersForGS()
	{
		perFrameConstantBuffer.setConstantBufferForGeometryShader(D3D::getInstancePtr()->getDeviceContext(), 0);
		gbufferConstantBuffer.setConstantBufferForGeometryShader(D3D::getInstancePtr()->getDeviceContext(), 3);
	}

	void Renderer::setPerFrameBuffersForPS()
	{
		perFrameConstantBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 0);
		gbufferConstantBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 3);
		m_fogRenderer.bindFogInfoForPS(4);
	}

	void Renderer::setPerFrameBuffersForCS()
	{
		perFrameConstantBuffer.setConstantBufferForComputeShader(D3D::getInstancePtr()->getDeviceContext(), 0);
	}

	void Renderer::setPerViewBuffersForVS()
	{
		perViewConstantBuffer.setConstantBufferForVertexShader(D3D::getInstancePtr()->getDeviceContext(), 1);
	}

	void Renderer::setPerViewBuffersForGS()
	{
		perViewConstantBuffer.setConstantBufferForGeometryShader(D3D::getInstancePtr()->getDeviceContext(), 1);
	}

	void Renderer::setPerViewBuffersForPS()
	{
		perViewConstantBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 1);
	}

	void Renderer::setPerViewBuffersForCS()
	{
		perViewConstantBuffer.setConstantBufferForComputeShader(D3D::getInstancePtr()->getDeviceContext(), 1);
	}

	void Renderer::setPerFrameGlobalSamplersForPS()
	{
		ID3D11SamplerState* samplers[] = { 
			m_samplerPointWrap.ptr(),
			m_samplerBilinearWrap.ptr(),
			m_samplerTrilinearWrap.ptr(),
			m_samplerAnisotropicWrap.ptr(),
			m_samplerDepth.ptr(),
			m_samplerBilinearClamp.ptr()
		};
	
		D3D::getInstancePtr()->getDeviceContext()->PSSetSamplers(0, 6, samplers);
	}

	void Renderer::setPerFrameGlobalSamplersForCS()
	{
		ID3D11SamplerState* samplers[] = {
			m_samplerPointWrap.ptr(),
			m_samplerBilinearWrap.ptr(),
			m_samplerTrilinearWrap.ptr(),
			m_samplerAnisotropicWrap.ptr(),
			m_samplerDepth.ptr(),
			m_samplerBilinearClamp.ptr()
		};

		D3D::getInstancePtr()->getDeviceContext()->CSSetSamplers(0, 6, samplers);
	}

	void Renderer::setSceneGBufferDepthSRVForPS(int slot)
	{
		auto* depthSRV = m_depthSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(slot, 1, &depthSRV);
	}

	void Renderer::unsetSceneDepthSRVForPS(int slot)
	{
		ID3D11ShaderResourceView* nullSRV[] = { NULL };
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(slot, 1, nullSRV);
	}

	void Renderer::setSceneGBufferDepthSRVForCS(int slot)
	{
		auto* depthSRV = m_depthSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->CSSetShaderResources(slot, 1, &depthSRV);
	}

	void Renderer::unsetSceneDepthSRVForCS(int slot)
	{
		ID3D11ShaderResourceView* nullSRV[] = { NULL };
		D3D::getInstancePtr()->getDeviceContext()->CSSetShaderResources(slot, 1, nullSRV);
	}

	void Renderer::updateBackbuffer(DxResPtr<ID3D11Texture2D> backbufferTexture)
	{
		createBackbuffer(backbufferTexture);
	}

	void Renderer::updateGBuffer()
	{
		initGBuffer();
	}

	void Renderer::releaseBackbufferResources()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(0, 0, 0);

		m_depthStencilBuffer.release();
		m_DSSDeptEnabledStencilDisabled.release();
		m_DSSDeptEnabledStencilReadWrite.release();
		m_DSSDeptEnabledStencilReadOnly.release();
		m_depthStencilViewReadWrite.release();
		m_depthStencilViewReadOnly.release();
		m_depthSRV.release();

		m_backbufferTexture.release();
		m_backbufferRTV.release();

		m_ldrTexture.release();
		m_ldrRTV.release();

		m_hdrTexture.release();
		m_hdrRTV.release();
	}

	void Renderer::setClearColor(const math::Vec3f& color)
	{
		m_clearColor = color;
	}

	void Renderer::setSkyTexture(std::shared_ptr<Texture> texture)
	{
		m_skyRenderer.init(texture);
	}

	void Renderer::setIBLTextures(std::shared_ptr<Texture> diffuseIBL, std::shared_ptr<Texture> specularIBL, std::shared_ptr<Texture> factorIBL)
	{
		m_diffuseIBLTexture = diffuseIBL;
		m_specularIBLTexture = specularIBL;
		m_factorIBLTexture = factorIBL;
	}

	void Renderer::setEV100(float EV100)
	{
		m_ev100 = EV100;
	}

	void Renderer::setFXAAQualitySubpix(float quality)
	{
		fxaaQualitySubpix = quality;
		postProcess.setFXAAQualitySubpix(fxaaQualitySubpix);
	}

	void Renderer::setFXAAQualityEdgeThreshhold(float quality)
	{
		fxaaQualityEdgeThreshold = quality;
		postProcess.setFXAAEdgeThreshhold(fxaaQualityEdgeThreshold);
	}

	void Renderer::setFXAAQualityEdgeThreshholdMin(float quality)
	{
		fxaaQualityEdgeThresholdMin = quality;
		postProcess.setFXAAEdgeThreshholdMin(fxaaQualityEdgeThresholdMin);
	}

	float Renderer::getFXAAQualitySubpix() const
	{
		return fxaaQualitySubpix;
	}

	float Renderer::getFXAAQualityEdgeThreshhold() const
	{
		return fxaaQualityEdgeThreshold;
	}

	float Renderer::getFXAAQualityEdgeThreshholdMin() const
	{
		return fxaaQualityEdgeThresholdMin;
	}

	void Renderer::prerenderIBLTextures(const std::wstring& diffuseTexFilePath, const std::wstring& specularTexFilePath, const std::wstring& reflectanceTexFilePath)
	{
		ReflectionCapture reflectionCapture;

		reflectionCapture.setEnvironmentTexture(m_skyRenderer.getSkyTexture());
		reflectionCapture.captureDiffuseReflection(diffuseTexFilePath);
		reflectionCapture.captureSpecularReflection(specularTexFilePath, reflectanceTexFilePath);

		auto result =  D3D::getInstancePtr()->getDevice()->GetDeviceRemovedReason();

		D3D11_TEXTURE2D_DESC backbufferDesc = {};
		m_ldrTexture->GetDesc(&backbufferDesc);

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = backbufferDesc.Width;
		viewport.Height = backbufferDesc.Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		
		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);
	}

	void Renderer::setRenderTarget(DxResPtr<ID3D11RenderTargetView> renderTagetView, DxResPtr<ID3D11DepthStencilView> depthStencilView)
	{
		auto* rtv = renderTagetView.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, depthStencilView);
	}

	void Renderer::setViewport(D3D11_VIEWPORT viewport)
	{
		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);
	}

	void Renderer::adjustViewportToBackbuffer()
	{
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = m_backbufferResolution.x();
		viewport.Height = m_backbufferResolution.y();
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);
	}

	void Renderer::setShouldUseDiffuseReflection(bool shouldUse)
	{
		useDiffuseReflection = shouldUse;
	}

	void Renderer::setShouldUseSpecularReflection(bool shouldUse)
	{
		useSpecularReflection = shouldUse;
	}

	void Renderer::setShouldUseIBL(bool shouldUse)
	{
		useIBL = shouldUse;
	}

	void Renderer::setShouldOverwriteRoughness(bool shouldOverwrite)
	{
		useRoughnessOverwriting = shouldOverwrite;
	}

	void Renderer::setOverwrittenRoughnessValue(float roughness)
	{
		overwrittenRoughness = roughness;
	}

	bool Renderer::shouldUseDiffuseReflection() const
	{
		return useDiffuseReflection;
	}

	bool Renderer::shouldUseSpecularReflection() const
	{
		return useSpecularReflection;
	}

	bool Renderer::shouldUseIBL() const
	{
		return useIBL;
	}

	bool Renderer::shouldOverwriteRoughness() const
	{
		return useRoughnessOverwriting;
	}

	float Renderer::getOverwrittenRoughnessValue() const
	{
		return overwrittenRoughness;
	}

	int Renderer::getDirectionalLightShadowResolution() const
	{
		return m_directionalLightResolution;
	}

	int Renderer::getPointLightShadowResolution() const
	{
		return m_pointLightResolution;
	}

	int Renderer::getSpotLightShadowResolution() const
	{
		return m_spotLightResolution;
	}

	void Renderer::setDirectionalLightShadowResolution(int resolution)
	{
		m_directionalLightResolution = resolution;
	}

	void Renderer::setPointLightShadowResolution(int resolution)
	{
		m_pointLightResolution = resolution;
	}

	void Renderer::setSpotLightShadowResolution(int resolution)
	{
		m_spotLightResolution = resolution;
	}

	void Renderer::enableBlending()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateBlendingEnabled, nullptr, 0xFFFFFFFF);
	}

	void Renderer::enableAlphaToCoverage()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateAlphaToCoverageEnabled, nullptr, 0xFFFFFFFF);
	}

	void Renderer::enableAdditiveBlending()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateAdditiveBlendingEnabled, nullptr, 0xFFFFFFFF);
	}

	void Renderer::disableBlending()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	}

	void Renderer::switchToGBufferBlending()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateGBuffer, nullptr, 0xFFFFFFFF);
	}

	void Renderer::switchToGBufferBlendingWithAlphaToCoverage()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateGBufferAlphaToCoverage, nullptr, 0xFFFFFFFF);
	}

	void Renderer::switchToDepthEnabledStencilDisabledState()
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(m_DSSDeptEnabledStencilDisabled, 0);
	}

	void Renderer::setReadWriteStencilRefValue(unsigned int value)
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(m_DSSDeptEnabledStencilReadWrite, value);
	}

	void Renderer::setReadOnlyStencilRefValue(unsigned int value)
	{
		D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(m_DSSDeptDisabledStencilReadOnly, value);
	}

	void Renderer::setFrontFaceCulling()
	{
		D3D::getInstancePtr()->getDeviceContext()->RSSetState(m_rasterizerStateFrontCulling);
	}

	void Renderer::setBackFaceCulling()
	{
		D3D::getInstancePtr()->getDeviceContext()->RSSetState(m_rasterizerState);
	}

	Renderer::GBuffer& Renderer::getGBuffer()
	{
		return m_GBuffer;
	}

	void Renderer::setFogDensity(float density)
	{
		m_fogRenderer.setDensity(density);
	}

	float Renderer::getFogDensity()
	{
		return m_fogRenderer.getDensity();
	}

	void Renderer::setFogPhaseFunctionParameter(float phase)
	{
		m_fogRenderer.setPhaseFunctionParameter(phase);
	}

	float Renderer::getFogPhaseFunctionParameter()
	{
		return m_fogRenderer.getPhaseFunctionParameter();
	}

	void Renderer::enableUniformFog()
	{
		m_fogRenderer.enableUniformFog();
	}

	bool Renderer::isUniformFogEnabled() const
	{
		return m_fogRenderer.isUniformFogEnabled();
	}

	void Renderer::enableVolumetricFog()
	{
		m_fogRenderer.enableVolumetricFog();
	}

	bool Renderer::isVolumetricFogEnabled() const
	{
		return m_fogRenderer.isVolumetricFogEnabled();
	}

	void Renderer::disableFog()
	{
		m_fogRenderer.disableFog();
	}

	bool Renderer::isFogEnabled() const
	{
		return m_fogRenderer.isFogEnabled();
	}

	void Renderer::bindVolumetricFogTextureForPS(int slot)
	{
		m_fogRenderer.bindVolumetricFogTextureForPS(slot);
	}

	void Renderer::clearViews()
	{
		float color[4] = { m_clearColor.x(), m_clearColor.y(), m_clearColor.z(), 1.0f };
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		
		devcon->ClearRenderTargetView(m_ldrRTV, color);
		devcon->ClearRenderTargetView(m_hdrRTV, color);
		devcon->ClearDepthStencilView(m_depthStencilViewReadWrite, D3D11_CLEAR_DEPTH, 0.0f, 0);
		devcon->ClearDepthStencilView(m_depthStencilViewReadWrite, D3D11_CLEAR_STENCIL, 0.0f, 0);
		
		devcon->ClearRenderTargetView(depthDirLightRTV, color);
		devcon->ClearRenderTargetView(depthSpotLightRTV, color);
		devcon->ClearRenderTargetView(depthCubemapRTV, color);
		
		devcon->ClearDepthStencilView(depthDirLightDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);
		devcon->ClearDepthStencilView(depthSpotLightDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);
		devcon->ClearDepthStencilView(depthCubemapDSV, D3D11_CLEAR_DEPTH, 0.0f, 0);

		devcon->ClearRenderTargetView(m_GBuffer.albedoRTV, color);
		devcon->ClearRenderTargetView(m_GBuffer.roughness_metalnessRTV, color);
		devcon->ClearRenderTargetView(m_GBuffer.normalRTV, color);
		devcon->ClearRenderTargetView(m_GBuffer.emissionRTV, color);
		devcon->ClearRenderTargetView(m_GBuffer.objectIDRTV, color);

		devcon->ClearDepthStencilView(m_GBuffer.m_depthStencilViewReadWrite, D3D11_CLEAR_DEPTH, 0.0f, 0);
		devcon->ClearDepthStencilView(m_GBuffer.m_depthStencilViewReadWrite, D3D11_CLEAR_STENCIL, 0.0f, 0);
	}

	void Renderer::renderDepth(Camera& camera)
	{
		D3D::getInstancePtr()->getDeviceContext()->RSSetState(m_depthRasterizerState);
		{
			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = m_directionalLightResolution;
			viewport.Height = m_directionalLightResolution;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

			auto& light = LightSystem::getInstancePtr()->getDirectionalLight();
			auto rtv = depthDirLightRTV.ptr();
			D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, depthDirLightDSV);
			D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(depthDirLightDSState, 0);

			updatePerViewData(light.depthCamera, false);
			this->setPerViewBuffersForVS();

			MeshSystem::getInstancePtr()->renderDepth2D();
		}
		{
			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = m_spotLightResolution;
			viewport.Height = m_spotLightResolution;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

			const auto& light = LightSystem::getInstancePtr()->getSpotLight();
			auto rtv = depthSpotLightRTV.ptr();
			D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, depthSpotLightDSV);
			D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(depthSpotLightDSState, 0);

			updatePerViewData(light.depthCamera, false);
			this->setPerViewBuffersForVS();

			MeshSystem::getInstancePtr()->renderDepth2D();
		}
		{
			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = m_pointLightResolution;
			viewport.Height = m_pointLightResolution;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

			initDepthCubemapRendering();

			auto rtv = depthCubemapRTV.ptr();
			D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, depthCubemapDSV);
			D3D::getInstancePtr()->getDeviceContext()->OMSetDepthStencilState(depthCubemapDSState, 0);

			auto camPos = camera.position();
			std::vector<math::Vec3f> positions;

			auto& pointLights = LightSystem::getInstancePtr()->getPointLights();
			for (auto& light : pointLights)
			{
				math::Vec3f pos = light.position;
				auto& trMat = TransformSystem::getInstance()->getMatrix(light.transformMatrixID);

				pos = (math::Vec4f(pos.x(), pos.y(), pos.z(), 1.0f) * trMat).head<3>();

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
				pos -= camera.position();
#endif

				positions.push_back(pos);
			}

			MeshSystem::getInstancePtr()->renderDepthCubemaps(positions);
		}

		D3D11_TEXTURE2D_DESC backbufferDesc = {};
		m_ldrTexture->GetDesc(&backbufferDesc);

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = backbufferDesc.Width;
		viewport.Height = backbufferDesc.Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);
	}

	void Renderer::renderStencil(Camera& camera)
	{
		auto* hdrRTV = m_hdrRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &hdrRTV, m_depthStencilViewReadWrite);

		updatePerViewData(camera);

		MeshSystem::getInstancePtr()->renderStencil();
	}

	void Renderer::renderGBufferGeometry(Camera& camera)
	{
		ID3D11RenderTargetView* RTVs[] = {
			m_GBuffer.albedoRTV.ptr(),
			m_GBuffer.roughness_metalnessRTV.ptr(),
			m_GBuffer.normalRTV.ptr(),
			m_GBuffer.emissionRTV.ptr(),
			m_GBuffer.objectIDRTV.ptr()
		};
		
		auto& particlesRingBuffer = ParticleSystem::getInstance()->getParticleRingBuffer();

		ID3D11UnorderedAccessView* UAVs[] = {
			particlesRingBuffer.m_gpuParticlesBuffer.getRWBufferUAV(),
			particlesRingBuffer.m_rangeBuffer.getRWBufferUAV()
		};

		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargetsAndUnorderedAccessViews(m_GBuffer.BUFFERS_COUNT, RTVs, m_GBuffer.m_depthStencilViewReadWrite, m_GBuffer.BUFFERS_COUNT, 2, UAVs, nullptr);
		D3D::getInstancePtr()->getDeviceContext()->OMSetBlendState(m_blendStateGBuffer, nullptr, 0xFFFFFFFF);
		updatePerViewData(camera);
		//switchToDepthEnabledStencilDisabledState();
		MeshSystem::getInstancePtr()->renderGBufferGeometry();
	}

	void Renderer::renderDecals(Camera& camera)
	{
		ID3D11RenderTargetView* RTVs[] = {
			m_GBuffer.albedoRTV.ptr(),
			m_GBuffer.roughness_metalnessRTV.ptr(),
			m_GBuffer.normalRTV.ptr(),
			m_GBuffer.emissionRTV.ptr(),
			m_GBuffer.objectIDRTV.ptr()
		};
		setReadOnlyStencilRefValue(1);
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(m_GBuffer.BUFFERS_COUNT, RTVs, m_GBuffer.m_depthStencilViewReadOnly);
		DecalSystem::getInstance()->render(camera);
	}


	void Renderer::renderGBufferLighing(Camera& camera)
	{
		auto* rtv = m_hdrRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtv, m_GBuffer.m_depthStencilViewReadOnly);
		
		auto* srvDir = depthDirLightSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(4, 1, &srvDir);
		auto* srvPoint = depthCubemapSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(5, 1, &srvPoint);
		auto* srvSpot = depthSpotLightSRV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(6, 1, &srvSpot);

		ID3D11ShaderResourceView* SRVs[] = {
			m_GBuffer.albedoSRV.ptr(),
			m_GBuffer.roughness_metalnessSRV.ptr(),
			m_GBuffer.normalSRV.ptr(),
			m_GBuffer.emissionSRV.ptr(),
			m_GBuffer.objectIDSRV.ptr(),
			m_GBuffer.depthSRV.ptr()
		};
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, m_GBuffer.BUFFERS_COUNT + 1, SRVs);
		updatePerViewData(camera);

		setReadOnlyStencilRefValue(1);
		m_gbufferPBRLightingShader.bind();
		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
		updatePerViewData(camera);

		setReadOnlyStencilRefValue(2);
		m_gbufferNoLightingShader.bind();
		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);

		switchToDepthEnabledStencilDisabledState();

		ID3D11ShaderResourceView* const nullSRV[1] = { NULL };
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(21, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(22, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(23, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(24, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(25, 1, nullSRV);

		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(4, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(5, 1, nullSRV);
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(6, 1, nullSRV);
	}

	void Renderer::updatePerFrameData(const Camera& camera, const Camera& prevCamera, float deltaTime, float timeSinceStart)
	{
		auto devcon = D3D::getInstancePtr()->getDeviceContext();

		{
			auto& mappedResource = perFrameConstantBuffer.map(devcon);
			PerFrame* ptr = static_cast<PerFrame*>(mappedResource.pData);

		ptr->timeSinceStart = timeSinceStart;
		ptr->deltaTime = deltaTime;

			ptr->useDiffuseReflection = useDiffuseReflection;
			ptr->useSpecularReflection = useSpecularReflection;
			ptr->useIBL = useIBL;
			ptr->useRoughnessOverwriting = useRoughnessOverwriting;
			ptr->overwrittenRoughness = overwrittenRoughness;
			ptr->specularIBLTextureMipLevels = m_specularIBLTexture ? m_specularIBLTexture->getMipLevels() : 0;
			perFrameConstantBuffer.unmap(devcon);
		}

		if (useIBL)
		{
			if (m_diffuseIBLTexture)
			{
				m_diffuseIBLTexture->bindSRVForVS(1);
				m_diffuseIBLTexture->bindSRVForPS(1);
			}
			if (m_specularIBLTexture)
			{
				m_specularIBLTexture->bindSRVForVS(2);
				m_specularIBLTexture->bindSRVForPS(2);
			}
			if (m_factorIBLTexture)
			{
				m_factorIBLTexture->bindSRVForVS(3);
				m_factorIBLTexture->bindSRVForPS(3);
			}
		}

		LightSystem::getInstancePtr()->update(camera, prevCamera);

		{
			auto& mappedResource = gbufferConstantBuffer.map(devcon);
			GBufferInfo* ptr = static_cast<GBufferInfo*>(mappedResource.pData);

			ptr->gbufferTexturesSize = m_backbufferResolution;

			gbufferConstantBuffer.unmap(devcon);
		}
	}

	void Renderer::updatePerViewData(const Camera& camera, bool useCameraCenteredWSdataIfSet)
	{
		auto devcon = D3D::getInstancePtr()->getDeviceContext();
		auto& mappedResource = perViewConstantBuffer.map(devcon);
		PerView* ptr = static_cast<PerView*>(mappedResource.pData);

		math::Vec3f BL, TL, BR;
		camera.getCameraFrustumCornersDirections(BL, TL, BR);

		ptr->view = camera.getView();
		ptr->viewInv = camera.getViewInv();
		ptr->proj = camera.getProj();
		ptr->projInv = camera.getProjInv();
		ptr->viewProj = camera.getViewProj();
		ptr->viewProjInv = camera.getViewProjInv();
		ptr->camFrustumBL = BL;
		ptr->camFrustumTL = TL;
		ptr->camFrustumBR = BR;
		ptr->cameraZNear = camera.getZNear();
		ptr->cameraZFar = camera.getZFar();

		if (SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE && useCameraCenteredWSdataIfSet)
		{
			Camera cam = camera;
			cam.setWorldOffset({ 0.0f, 0.0f, 0.0f });
			cam.updateMatrices();
			ptr->view = cam.getView();
			ptr->viewInv = cam.getViewInv();
			ptr->proj = cam.getProj();
			ptr->projInv = cam.getProjInv();
			ptr->viewProj = cam.getViewProj();
			ptr->viewProjInv = cam.getViewProjInv();
		}

		ptr->cameraPosition = camera.position();

		perViewConstantBuffer.unmap(devcon);
	}

	void Renderer::createBackbuffer(DxResPtr<ID3D11Texture2D> backbufferTexture)
	{
		m_backbufferTexture = backbufferTexture;

		D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(m_backbufferTexture, NULL, m_backbufferRTV.reset());

		D3D11_TEXTURE2D_DESC backbufferDesc = {};
		m_backbufferTexture->GetDesc(&backbufferDesc);

		m_backbufferResolution = math::Vec2i(backbufferDesc.Width, backbufferDesc.Height);

		{
			D3D11_TEXTURE2D_DESC ldrRTVTextureDesc = backbufferDesc;
			ldrRTVTextureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			D3D::getInstancePtr()->getDevice()->CreateTexture2D(&ldrRTVTextureDesc, nullptr, m_ldrTexture.reset());

			D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(m_ldrTexture, NULL, m_ldrRTV.reset());
		}
		{
			D3D11_TEXTURE2D_DESC hdrRTVTextureDesc = backbufferDesc;
			hdrRTVTextureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
			hdrRTVTextureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			D3D::getInstancePtr()->getDevice()->CreateTexture2D(&hdrRTVTextureDesc, nullptr, m_hdrTexture.reset());

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(m_hdrTexture, &rtvDesc, m_hdrRTV.reset());
		}

		initDepthStencilBuffer(backbufferTexture);

		//D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, hdrRTV, m_depthStencilViewReadWrite);

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = backbufferDesc.Width;
		viewport.Height = backbufferDesc.Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);
	}

	void Renderer::initDepthStencilBuffer(DxResPtr<ID3D11Texture2D> backbufferTexture)
	{
		HRESULT result;
		auto d3d = D3D::getInstancePtr();

		D3D11_TEXTURE2D_DESC backbufferDesc = {};
		backbufferTexture->GetDesc(&backbufferDesc);

		D3D11_TEXTURE2D_DESC depthStencilTextureDesc = {};
		depthStencilTextureDesc.Width = backbufferDesc.Width;
		depthStencilTextureDesc.Height = backbufferDesc.Height;
		depthStencilTextureDesc.MipLevels = 1;
		depthStencilTextureDesc.ArraySize = 1;
		depthStencilTextureDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthStencilTextureDesc.SampleDesc.Count = 1;
		depthStencilTextureDesc.SampleDesc.Quality = 0;
		depthStencilTextureDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthStencilTextureDesc.CPUAccessFlags = 0;
		depthStencilTextureDesc.MiscFlags = 0;

		result = d3d->getDevice()->CreateTexture2D(&depthStencilTextureDesc, NULL, m_depthStencilBuffer.reset());
		ALWAYS_ASSERT(result >= 0);

		D3D11_SHADER_RESOURCE_VIEW_DESC depthSRVDesc{};
		depthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		depthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		depthSRVDesc.Texture2D.MipLevels = -1;
		depthSRVDesc.Texture2D.MostDetailedMip = 0;

		result = d3d->getDevice()->CreateShaderResourceView(m_depthStencilBuffer, &depthSRVDesc, m_depthSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
		
		result = d3d->getDevice()->CreateDepthStencilState(&depthStencilDesc, m_DSSDeptEnabledStencilDisabled.reset());
		ALWAYS_ASSERT(result >= 0);

		depthStencilDesc.StencilEnable = true;
		depthStencilDesc.StencilReadMask = 0xFF;
		depthStencilDesc.StencilWriteMask = 0xFF;

		depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
		depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

		result = d3d->getDevice()->CreateDepthStencilState(&depthStencilDesc, m_DSSDeptEnabledStencilReadWrite.reset());
		ALWAYS_ASSERT(result >= 0);

		depthStencilDesc.DepthEnable = false;

		depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

		depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

		result = d3d->getDevice()->CreateDepthStencilState(&depthStencilDesc, m_DSSDeptDisabledStencilReadOnly.reset());
		ALWAYS_ASSERT(result >= 0);

		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			
			result = d3d->getDevice()->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, m_depthStencilViewReadWrite.reset());
		}
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			depthStencilViewDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

			result = d3d->getDevice()->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, m_depthStencilViewReadOnly.reset());
		}
		
		{
			D3D11_RASTERIZER_DESC desc{};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.0f;
			desc.DepthClipEnable = true;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = true;
			desc.AntialiasedLineEnable = false;

			D3D::getInstancePtr()->getDevice()->CreateRasterizerState(&desc, m_rasterizerState.reset());

			desc.CullMode = D3D11_CULL_FRONT;
			D3D::getInstancePtr()->getDevice()->CreateRasterizerState(&desc, m_rasterizerStateFrontCulling.reset());
		}
		ALWAYS_ASSERT(result >= 0);
	}
	void Renderer::initSamplers()
	{
		HRESULT result;
		
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerPointWrap.reset());
		ALWAYS_ASSERT(result >= 0);

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerBilinearWrap.reset());
		ALWAYS_ASSERT(result >= 0);

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerTrilinearWrap.reset());
		ALWAYS_ASSERT(result >= 0);

		samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerAnisotropicWrap.reset());
		ALWAYS_ASSERT(result >= 0);

		samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_GREATER;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerDepth.reset());
		ALWAYS_ASSERT(result >= 0);

		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		result = D3D::getInstancePtr()->getDevice()->CreateSamplerState(&samplerDesc, m_samplerBilinearClamp.reset());
		ALWAYS_ASSERT(result >= 0);
	}
	void Renderer::initBlendState()
	{
		D3D11_BLEND_DESC1 desc{};
		desc.RenderTarget[0].BlendEnable = true;
		desc.RenderTarget[0].LogicOpEnable = false;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].LogicOp = D3D11_LOGIC_OP_NOOP;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		auto result = D3D::getInstancePtr()->getDevice()->CreateBlendState1(&desc, m_blendStateBlendingEnabled.reset());
		ALWAYS_ASSERT(result >= 0);

		desc.AlphaToCoverageEnable = true;
		result = D3D::getInstancePtr()->getDevice()->CreateBlendState1(&desc, m_blendStateAlphaToCoverageEnabled.reset());
		ALWAYS_ASSERT(result >= 0);

		desc.AlphaToCoverageEnable = false;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		result = D3D::getInstancePtr()->getDevice()->CreateBlendState1(&desc, m_blendStateAdditiveBlendingEnabled.reset());
		ALWAYS_ASSERT(result >= 0);
	}
	void Renderer::initGBuffer()
	{
		ALWAYS_ASSERT(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT >= m_GBuffer.BUFFERS_COUNT);

		auto* device = D3D::getInstancePtr()->getDevice();
		HRESULT result;

		D3D11_TEXTURE2D_DESC backbufferDesc;
		m_ldrTexture->GetDesc(&backbufferDesc);

		D3D11_TEXTURE2D_DESC texDesc{};
		texDesc.Width = backbufferDesc.Width;
		texDesc.Height = backbufferDesc.Height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		texDesc.Format = rtvDesc.Format = srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.albedo.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateRenderTargetView(m_GBuffer.albedo, &rtvDesc, m_GBuffer.albedoRTV.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateShaderResourceView(m_GBuffer.albedo, &srvDesc, m_GBuffer.albedoSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		texDesc.Format = rtvDesc.Format = srvDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.roughness_metalness.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateRenderTargetView(m_GBuffer.roughness_metalness, &rtvDesc, m_GBuffer.roughness_metalnessRTV.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateShaderResourceView(m_GBuffer.roughness_metalness, &srvDesc, m_GBuffer.roughness_metalnessSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		texDesc.Format = rtvDesc.Format = srvDesc.Format = DXGI_FORMAT_R16G16B16A16_SNORM;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.normal.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateRenderTargetView(m_GBuffer.normal, &rtvDesc, m_GBuffer.normalRTV.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateShaderResourceView(m_GBuffer.normal, &srvDesc, m_GBuffer.normalSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		texDesc.Format = rtvDesc.Format = srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.emission.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateRenderTargetView(m_GBuffer.emission, &rtvDesc, m_GBuffer.emissionRTV.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateShaderResourceView(m_GBuffer.emission, &srvDesc, m_GBuffer.emissionSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		texDesc.Format = rtvDesc.Format = srvDesc.Format = DXGI_FORMAT_R32_UINT;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.objectID.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateRenderTargetView(m_GBuffer.objectID, &rtvDesc, m_GBuffer.objectIDRTV.reset());
		ALWAYS_ASSERT(result >= 0);
		result = device->CreateShaderResourceView(m_GBuffer.objectID, &srvDesc, m_GBuffer.objectIDSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		texDesc.Format =  DXGI_FORMAT_R24G8_TYPELESS;
		texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		result = device->CreateTexture2D(&texDesc, nullptr, m_GBuffer.depthStencil.reset());
		ALWAYS_ASSERT(result >= 0);
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		result = device->CreateShaderResourceView(m_GBuffer.depthStencil, &srvDesc, m_GBuffer.depthSRV.reset());
		ALWAYS_ASSERT(result >= 0);

		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;

			result = device->CreateDepthStencilView(m_GBuffer.depthStencil, &depthStencilViewDesc, m_GBuffer.m_depthStencilViewReadWrite.reset());
		}
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			depthStencilViewDesc.Flags = D3D11_DSV_READ_ONLY_DEPTH;

			result = device->CreateDepthStencilView(m_GBuffer.depthStencil, &depthStencilViewDesc, m_GBuffer.m_depthStencilViewReadOnly.reset());
		}

		m_gbufferPBRLightingShader.init(L"Shaders/GBuffer/gbufferLightingVS.hlsl", L"Shaders/GBuffer/gbufferPBRLightingPS.hlsl", {});
		m_gbufferNoLightingShader.init(L"Shaders/GBuffer/gbufferLightingVS.hlsl", L"Shaders/GBuffer/gbufferNoLightingPS.hlsl", {});

		D3D11_BLEND_DESC1 bsDesc{};
		bsDesc.AlphaToCoverageEnable = false;
		bsDesc.IndependentBlendEnable = true;
		for (int i = 0; i < m_GBuffer.BUFFERS_COUNT; i++)
		{
			if (i == 2 || i == 4) // RTV for normals and objectID
			{
				bsDesc.RenderTarget[i].BlendEnable = false;
			}
			else
			{
				bsDesc.RenderTarget[i].BlendEnable = true;
			}
			bsDesc.RenderTarget[i].LogicOpEnable = false;
			bsDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bsDesc.RenderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bsDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
			bsDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
			bsDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
			bsDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bsDesc.RenderTarget[i].LogicOp = D3D11_LOGIC_OP_NOOP;
			bsDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		result = D3D::getInstancePtr()->getDevice()->CreateBlendState1(&bsDesc, m_blendStateGBuffer.reset());
		ALWAYS_ASSERT(result >= 0);

		bsDesc.AlphaToCoverageEnable = true;
		result = D3D::getInstancePtr()->getDevice()->CreateBlendState1(&bsDesc, m_blendStateGBufferAlphaToCoverage.reset());
		ALWAYS_ASSERT(result >= 0);

		gbufferConstantBuffer.createConstantBuffer(device);
	}
	void Renderer::initDepthRendering()
	{
		initDepth2DRendering();
		initDepthCubemapRendering(true);
	}
	void Renderer::initDepth2DRendering()
	{
		auto* device = D3D::getInstancePtr()->getDevice();
		//auto* devCon = D3D::getInstancePtr()->getDeviceContext();

		D3D11_TEXTURE2D_DESC texDesc;
		m_ldrTexture->GetDesc(&texDesc);
		//auto width = texDesc.Width, height = texDesc.Height;
		texDesc.Width = m_directionalLightResolution;
		texDesc.Height = m_directionalLightResolution;

		DxResPtr<ID3D11Texture2D> depthDirRTVTex, depthSpotRTVTex;
		device->CreateTexture2D(&texDesc, nullptr, depthDirRTVTex.reset());

		texDesc.Width = m_spotLightResolution;
		texDesc.Height = m_spotLightResolution;
		device->CreateTexture2D(&texDesc, nullptr, depthSpotRTVTex.reset());

		device->CreateRenderTargetView(depthDirRTVTex, nullptr, depthDirLightRTV.reset());
		device->CreateRenderTargetView(depthSpotRTVTex, nullptr, depthSpotLightRTV.reset());

		D3D11_TEXTURE2D_DESC depthTexDesc{};
		depthTexDesc.Width = m_directionalLightResolution;
		depthTexDesc.Height = m_directionalLightResolution;
		depthTexDesc.MipLevels = 1;
		depthTexDesc.ArraySize = 1;
		depthTexDesc.SampleDesc.Count = 1;
		depthTexDesc.SampleDesc.Quality = 0;
		depthTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		device->CreateTexture2D(&depthTexDesc, nullptr, depthDirLightTexture.reset());

		depthTexDesc.Width = m_spotLightResolution;
		depthTexDesc.Height = m_spotLightResolution;
		device->CreateTexture2D(&depthTexDesc, nullptr, depthSpotLightTexture.reset());

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

		device->CreateDepthStencilState(&depthStencilDesc, depthDirLightDSState.reset());
		device->CreateDepthStencilState(&depthStencilDesc, depthSpotLightDSState.reset());

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		device->CreateDepthStencilView(depthDirLightTexture, &depthStencilViewDesc, depthDirLightDSV.reset());
		device->CreateDepthStencilView(depthSpotLightTexture, &depthStencilViewDesc, depthSpotLightDSV.reset());

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		device->CreateShaderResourceView(depthDirLightTexture, &srvDesc, depthDirLightSRV.reset());
		device->CreateShaderResourceView(depthSpotLightTexture, &srvDesc, depthSpotLightSRV.reset());

		{
			D3D11_RASTERIZER_DESC desc{};
			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = false;
			desc.DepthBias = -16;
			desc.DepthBiasClamp = 0.0f;
			desc.SlopeScaledDepthBias = -8.0f;
			desc.DepthClipEnable = true;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = false;
			desc.AntialiasedLineEnable = false;

			auto result = D3D::getInstancePtr()->getDevice()->CreateRasterizerState(&desc, m_depthRasterizerState.reset());
			ALWAYS_ASSERT(result >= 0);
		}
	}
	void Renderer::initDepthCubemapRendering(bool first)
	{
		auto* device = D3D::getInstancePtr()->getDevice();

		int prevSize = depthCubemapsCount;

		auto* lighSystem = LightSystem::getInstancePtr();
		if (lighSystem)
		{
			int pointLightsCount = lighSystem->getPointLights().size();
			if (pointLightsCount != 0)
			{
				depthCubemapsCount = std::min(pointLightsCount, MAX_POINT_LIGHTS);
			}
		}

		if (!first && prevSize == depthCubemapsCount)
			return;

		int arraySize = depthCubemapsCount * 6;

		D3D11_TEXTURE2D_DESC texDesc;
		m_ldrTexture->GetDesc(&texDesc);
		texDesc.Width = m_pointLightResolution;
		texDesc.Height = m_pointLightResolution;
		texDesc.ArraySize = arraySize;

		DxResPtr<ID3D11Texture2D> depthRTVTex;
		device->CreateTexture2D(&texDesc, nullptr, depthRTVTex.reset());

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = texDesc.Format;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = arraySize;

		device->CreateRenderTargetView(depthRTVTex, &rtvDesc, depthCubemapRTV.reset());

		D3D11_TEXTURE2D_DESC depthTexDesc{};
		depthTexDesc.Width = m_pointLightResolution;
		depthTexDesc.Height = m_pointLightResolution;
		depthTexDesc.MipLevels = 1;
		depthTexDesc.ArraySize = arraySize;
		depthTexDesc.SampleDesc.Count = 1;
		depthTexDesc.SampleDesc.Quality = 0;
		depthTexDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		depthTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		depthTexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		device->CreateTexture2D(&depthTexDesc, nullptr, depthCubemapTexture.reset());

		D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
		depthStencilDesc.DepthEnable = true;
		depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;

		device->CreateDepthStencilState(&depthStencilDesc, depthCubemapDSState.reset());

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
		depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		depthStencilViewDesc.Texture2DArray.MipSlice = 0;
		depthStencilViewDesc.Texture2DArray.ArraySize = arraySize;

		device->CreateDepthStencilView(depthCubemapTexture, &depthStencilViewDesc, depthCubemapDSV.reset());

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		srvDesc.TextureCubeArray.MipLevels = 1;
		srvDesc.TextureCubeArray.MostDetailedMip = 0;
		srvDesc.TextureCubeArray.NumCubes = depthCubemapsCount;

		device->CreateShaderResourceView(depthCubemapTexture, &srvDesc, depthCubemapSRV.reset());
	}
}