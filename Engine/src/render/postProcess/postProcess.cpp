#include "postProcess.h"
#include "../../engine/renderer.h"

void Engine::PostProcess::init()
{
	m_hdr2ldrShader.init(L"Shaders/postProcess/postProcessVS.hlsl", L"Shaders/postProcess/postProcessPS.hlsl", {});
	m_hdr2ldrParamsCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());

	m_hdr2ldrParams.gamma = 2.2f;
	m_hdr2ldrParams.ev100 = 1.0f;

	m_fxaaShader.init(L"Shaders/postProcess/fxaaVS.hlsl", L"Shaders/postProcess/fxaaPS.hlsl", {});
	m_fxaaParamsCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());

	m_fxaaParams.qualitySubpix = 0.75f;
	m_fxaaParams.qualityEdgeThreshold = 0.063f;
	m_fxaaParams.qualityEdgeThresholdMin = 0.0312f;

	m_bloomDownsampleShader.init(L"Shaders/postProcess/bloomVS.hlsl", L"Shaders/postProcess/bloomDownsamplePS.hlsl", {});
	m_bloomDownsampleCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	m_bloomUpsampleShader.init(L"Shaders/postProcess/bloomVS.hlsl", L"Shaders/postProcess/bloomUpsamplePS.hlsl", {});
	m_bloomUpsampleCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	m_bloomFinalShader.init(L"Shaders/postProcess/bloomVS.hlsl", L"Shaders/postProcess/bloomFinalPS.hlsl", {});
	
	m_bloomUpsampleInfo.filterRadius = 0.005f;
}

void Engine::PostProcess::resolve(DxResPtr<ID3D11RenderTargetView> src, DxResPtr<ID3D11RenderTargetView> dst)
{
	DxResPtr<ID3D11Texture2D> srcTexture;
	src->GetResource((ID3D11Resource**)&srcTexture);

	D3D11_TEXTURE2D_DESC textureDesc;
	srcTexture->GetDesc(&textureDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(srcTexture, nullptr, m_srv.reset());
	
	auto* devcon = D3D::getInstancePtr()->getDeviceContext();
	m_hdr2ldrShader.bind();
	
	auto res = m_hdr2ldrParamsCBuffer.map(devcon);
	
	Hdr2LdrParams* ptr = static_cast<Hdr2LdrParams*>(res.pData);
	ptr->gamma = m_hdr2ldrParams.gamma;
	ptr->ev100 = m_hdr2ldrParams.ev100;

	m_hdr2ldrParamsCBuffer.unmap(devcon);

	m_hdr2ldrParamsCBuffer.setConstantBufferForPixelShader(devcon, 10);

	auto* SRV = m_srv.ptr();
	devcon->PSSetShaderResources(20, 1, &SRV);
	
	Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();
	
	devcon->Draw(3, 0);
	
	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	devcon->PSSetShaderResources(20, 1, pSRV);
}

void Engine::PostProcess::applyFXAA(DxResPtr<ID3D11RenderTargetView> src, DxResPtr<ID3D11RenderTargetView> dst)
{
	DxResPtr<ID3D11Texture2D> srcTexture;
	src->GetResource((ID3D11Resource**)&srcTexture);

	D3D11_TEXTURE2D_DESC textureDesc;
	srcTexture->GetDesc(&textureDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(srcTexture, nullptr, m_srv.reset());

	auto* devcon = D3D::getInstancePtr()->getDeviceContext();
	m_fxaaShader.bind();

	auto res = m_fxaaParamsCBuffer.map(devcon);

	FxaaParams* ptr = static_cast<FxaaParams*>(res.pData);
	ptr->imageSize = math::Vec4f(textureDesc.Width, textureDesc.Height, 1.0f / textureDesc.Width, 1.0f / textureDesc.Height);
	ptr->qualitySubpix = m_fxaaParams.qualitySubpix;
	ptr->qualityEdgeThreshold = m_fxaaParams.qualityEdgeThreshold;
	ptr->qualityEdgeThresholdMin = m_fxaaParams.qualityEdgeThresholdMin;

	m_fxaaParamsCBuffer.unmap(devcon);

	m_fxaaParamsCBuffer.setConstantBufferForPixelShader(devcon, 10);

	auto* SRV = m_srv.ptr();
	devcon->PSSetShaderResources(20, 1, &SRV);

	Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

	devcon->Draw(3, 0);

	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	devcon->PSSetShaderResources(20, 1, pSRV);
}

void Engine::PostProcess::applyBloom(const DxResPtr<ID3D11RenderTargetView>& src, DxResPtr<ID3D11RenderTargetView>& dst, int bloomChainLength)
{
	if (bloomChainLength < 1)
	{
		return;
	}

	initBloomChain(src, bloomChainLength);
	bloomDownsample(src, bloomChainLength);
	bloomUpsample(dst, bloomChainLength);

	Renderer::getInstancePtr()->adjustViewportToBackbuffer();
}

void Engine::PostProcess::initBloomChain(const DxResPtr<ID3D11RenderTargetView>& src, int bloomChainLength)
{
	m_bloomChain.textures.clear();
	m_bloomChain.sizes.clear();

	DxResPtr<ID3D11Texture2D> texture;
	src->GetResource((ID3D11Resource**)(texture.reset()));

	D3D11_TEXTURE2D_DESC texDesc;
	texture->GetDesc(&texDesc);

	math::Vec2i prevSize(texDesc.Width, texDesc.Height);

	m_bloomChain.textures.push_back(texture);
	m_bloomChain.sizes.push_back(prevSize);

	for (int i = 0; i < bloomChainLength; i++)
	{
		math::Vec2i downsampleSize = prevSize / 2;
		downsampleSize = downsampleSize.cwiseMax(1);

		if (downsampleSize == prevSize)
		{
			//reached size of 1x1
			break;
		}
		prevSize = downsampleSize;

		texDesc.Width = downsampleSize.x();
		texDesc.Height = downsampleSize.y();
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&texDesc, nullptr, texture.reset());

		m_bloomChain.textures.push_back(texture);
		m_bloomChain.sizes.push_back(downsampleSize);
	}
}

void Engine::PostProcess::bloomDownsample(const DxResPtr<ID3D11RenderTargetView>& src, int bloomChainLength)
{
	for (int i = 1; i < m_bloomChain.textures.size(); i++)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		src->GetDesc(&rtvDesc);

		D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(m_bloomChain.textures[i], &rtvDesc, m_bloomRTV.reset());

		ID3D11RenderTargetView* rtvPtr = m_bloomRTV.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtvPtr, nullptr);
		
		D3D11_VIEWPORT viewport{};
		viewport.Width = m_bloomChain.sizes[i].x();
		viewport.Height = m_bloomChain.sizes[i].y();
		viewport.MaxDepth = 1.0f;

		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

		m_bloomDownsampleShader.bind();
		Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

		auto res = m_bloomDownsampleCBuffer.map(D3D::getInstancePtr()->getDeviceContext());

		BloomDownsampleInfo* ptr = static_cast<BloomDownsampleInfo*>(res.pData);
		ptr->textureSize = m_bloomChain.sizes[i - 1];

		m_bloomDownsampleCBuffer.unmap(D3D::getInstancePtr()->getDeviceContext());
		m_bloomDownsampleCBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 10);

		DxResPtr<ID3D11ShaderResourceView> srv;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(m_bloomChain.textures[i - 1], nullptr, srv.reset());

		auto* srvPtr = srv.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 1, &srvPtr);

		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
	}

	ID3D11ShaderResourceView* const pSRV[1] = { NULL };
	D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 1, pSRV);
}

void Engine::PostProcess::bloomUpsample(DxResPtr<ID3D11RenderTargetView>& dst, int bloomChainLength)
{
	Renderer::getInstancePtr()->enableAdditiveBlending();

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
	dst->GetDesc(&rtvDesc);
	for (int i = m_bloomChain.textures.size() - 2; i > 0; i--)
	{
		DxResPtr<ID3D11RenderTargetView> rtv;
		D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(m_bloomChain.textures[i], nullptr, rtv.reset());

		ID3D11RenderTargetView* rtvPtr = rtv.ptr();
		D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtvPtr, nullptr);

		D3D11_VIEWPORT viewport{};
		viewport.Width = m_bloomChain.sizes[i].x();
		viewport.Height = m_bloomChain.sizes[i].y();
		viewport.MaxDepth = 1.0f;

		D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

		m_bloomUpsampleShader.bind();

		auto res = m_bloomUpsampleCBuffer.map(D3D::getInstancePtr()->getDeviceContext());

		BloomUpsampleInfo* ptr = static_cast<BloomUpsampleInfo*>(res.pData);
		ptr->filterRadius = m_bloomUpsampleInfo.filterRadius;

		m_bloomUpsampleCBuffer.unmap(D3D::getInstancePtr()->getDeviceContext());
		m_bloomUpsampleCBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 10);

		DxResPtr<ID3D11ShaderResourceView> srv;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(m_bloomChain.textures[i + 1], nullptr, srv.reset());

		auto* srvPtr = srv.ptr();
		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 1, &srvPtr);

		Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();
		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
	}
	Renderer::getInstancePtr()->disableBlending();
	
	D3D11_TEXTURE2D_DESC texDesc;
	m_bloomChain.textures[0]->GetDesc(&texDesc);

	DxResPtr<ID3D11Texture2D> tex;
	D3D::getInstancePtr()->getDevice()->CreateTexture2D(&texDesc, nullptr, tex.reset());
		
	D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(tex, nullptr, dst.reset());

	ID3D11RenderTargetView* rtvPtr = dst.ptr();
	D3D::getInstancePtr()->getDeviceContext()->OMSetRenderTargets(1, &rtvPtr, nullptr);

	D3D11_VIEWPORT viewport{};
	viewport.Width = m_bloomChain.sizes[0].x();
	viewport.Height = m_bloomChain.sizes[0].y();
	viewport.MaxDepth = 1.0f;

	D3D::getInstancePtr()->getDeviceContext()->RSSetViewports(1, &viewport);

	m_bloomFinalShader.bind();

	DxResPtr<ID3D11ShaderResourceView> srcSRV, prevSRV;
	D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(m_bloomChain.textures[1], nullptr, prevSRV.reset());
	D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(m_bloomChain.textures[0], nullptr, srcSRV.reset());

	ID3D11ShaderResourceView* prevSrvPtr = prevSRV.ptr();
	ID3D11ShaderResourceView* srcSrvPtr = srcSRV.ptr();
	D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 1, &prevSrvPtr);
	D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(21, 1, &srcSrvPtr);

	Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();
	D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);

	ID3D11ShaderResourceView* const pSRV[2] = { NULL , NULL };
	D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(20, 2, pSRV);
}
