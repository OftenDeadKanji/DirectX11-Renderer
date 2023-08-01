#include "reflectionCapture.h"
#include "../../dependencies/DirectXTex/DirectXTex/DirectXTex.h"
#include "../../utils/assert.h"
#include "../camera/camera.h"
#include "../../engine/renderer.h"

namespace Engine
{
	enum class FileFormat
	{
		NONE,
		PNG,
		TGA,
		HDR,
		BC1_LINEAR = DXGI_FORMAT_BC1_UNORM,			// RGB, 1 bit Alpha
		BC1_SRGB = DXGI_FORMAT_BC1_UNORM_SRGB,		// RGB, 1-bit Alpha, SRGB
		BC3_LINEAR = DXGI_FORMAT_BC3_UNORM,			// RGBA
		BC3_SRGB = DXGI_FORMAT_BC3_UNORM_SRGB,		// RGBA, SRGB
		BC4_UNSIGNED = DXGI_FORMAT_BC4_UNORM,		// GRAY, unsigned
		BC4_SIGNED = DXGI_FORMAT_BC4_SNORM,			// GRAY, signed
		BC5_UNSIGNED = DXGI_FORMAT_BC5_UNORM,		// RG, unsigned
		BC5_SIGNED = DXGI_FORMAT_BC5_SNORM,			// RG, signed
		BC6_UNSIGNED = DXGI_FORMAT_BC6H_UF16,		// RGB HDR, unsigned
		BC6_SIGNED = DXGI_FORMAT_BC6H_SF16,			// RGB HDR, signed
		BC7_LINEAR = DXGI_FORMAT_BC7_UNORM,			// RGBA Advanced
		BC7_SRGB = DXGI_FORMAT_BC7_UNORM_SRGB,		// RGBA Advanced, SRGB
	};

	void save(const std::wstring& fileName, DxResPtr<ID3D11Texture2D> texture, bool generateMips, FileFormat format)
	{
		DirectX::ScratchImage scratchImage;
		DirectX::CaptureTexture(D3D::getInstancePtr()->getDevice(), D3D::getInstancePtr()->getDeviceContext(), texture, scratchImage);

		const DirectX::ScratchImage* imagePtr = &scratchImage;

		DirectX::ScratchImage mipchain;
		if (generateMips)
		{
			DirectX::GenerateMipMaps(*scratchImage.GetImage(0, 0, 0), DirectX::TEX_FILTER_DEFAULT, 0, mipchain);
			imagePtr = &mipchain;
		}

		DirectX::ScratchImage compressed;
		if (DirectX::IsCompressed(DXGI_FORMAT(format)))
		{
			HRESULT result;
			if (FileFormat::BC6_UNSIGNED <= format && format <= FileFormat::BC7_SRGB)
				result = DirectX::Compress(D3D::getInstancePtr()->getDevice(), imagePtr->GetImages(), imagePtr->GetImageCount(), imagePtr->GetMetadata(),
					DXGI_FORMAT(format), DirectX::TEX_COMPRESS_PARALLEL, 1.f, compressed);
			else
				result = DirectX::Compress(imagePtr->GetImages(), imagePtr->GetImageCount(), imagePtr->GetMetadata(),
					DXGI_FORMAT(format), DirectX::TEX_COMPRESS_PARALLEL, 1.f, compressed);

			DEV_ASSERT(result >= 0);
			imagePtr = &compressed;
		}

		DirectX::SaveToDDSFile(imagePtr->GetImages(), imagePtr->GetImageCount(), imagePtr->GetMetadata(), DirectX::DDS_FLAGS(0), fileName.c_str());
	}

	ReflectionCapture::ReflectionCapture()
	{
		m_diffuseReflectionShader.init(L"Shaders/reflectionCapture/reflectionCaptureDiffuseVS.hlsl", L"Shaders/reflectionCapture/reflectionCaptureDiffusePS.hlsl", {});
		m_specularReflectionShader.init(L"Shaders/reflectionCapture/reflectionCaptureSpecularVS.hlsl", L"Shaders/reflectionCapture/reflectionCaptureSpecularPS.hlsl", {});
		m_reflectanceFactorShader.init(L"Shaders/reflectionCapture/reflectionCaptureFactorVS.hlsl", L"Shaders/reflectionCapture/reflectionCaptureFactorPS.hlsl", {});

		m_environmentTextureInfoCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	}

	ReflectionCapture::~ReflectionCapture()
	{
		m_environmentTextureInfoCBuffer.reset();
	}
	
	void ReflectionCapture::setEnvironmentTexture(std::shared_ptr<Texture> environmentTexture)
	{
		m_environmentTexture = environmentTexture;

		auto* devcon = D3D::getInstancePtr()->getDeviceContext();

		auto mappedRes = m_environmentTextureInfoCBuffer.map(devcon);
		static_cast<EnvironmentTextureInfo*>(mappedRes.pData)->size = m_environmentTexture->getSize();
		m_environmentTextureInfoCBuffer.unmap(devcon);
	}

	void ReflectionCapture::captureDiffuseReflection(std::wstring textureFileName)
	{
		Camera camera;
		camera.setPerspective(90.0f, 1.0f, 0.1f, 10.0f);

		math::Angles cameraAngles[6] = {
			math::Angles({0.0f, math::deg2rad(-90.0f), 0.0f}),
			math::Angles({0.0f, math::deg2rad(90.0f), 0.0f}),
			math::Angles({math::deg2rad(90.0f), 0.0f, 0.0f}),
			math::Angles({math::deg2rad(-90.0f), 0.0f, 0.0f}),
			math::Angles({0.0f, 0.0f ,0.0f}),
			math::Angles({0.0f, math::deg2rad(180.0f), 0.0f})
		};

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 1;

		D3D11_TEXTURE2D_DESC diffTexDesc{};
		diffTexDesc.Width = 8;
		diffTexDesc.Height = 8;
		diffTexDesc.MipLevels = 1;
		diffTexDesc.ArraySize = 6;
		diffTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		diffTexDesc.SampleDesc.Count = 1;
		diffTexDesc.SampleDesc.Quality = 0;
		diffTexDesc.Usage = D3D11_USAGE_DEFAULT;
		diffTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		diffTexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		DxResPtr<ID3D11Texture2D> diffuseTexture;
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&diffTexDesc, nullptr, diffuseTexture.reset());

		for (int i = 0; i < 6; i++)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = i;

			DxResPtr<ID3D11RenderTargetView> rtv;
			auto result = D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(diffuseTexture, &rtvDesc, rtv.reset());

			Renderer::getInstancePtr()->setRenderTarget(rtv, {});

			D3D11_VIEWPORT viewport = {};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = diffTexDesc.Width;
			viewport.Height = diffTexDesc.Height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			Renderer::getInstancePtr()->setViewport(viewport);

			//render
			camera.setWorldAngles(cameraAngles[i]);
			camera.updateCamera();

			Renderer::getInstancePtr()->updatePerFrameData(camera, camera, 0.0f, 0.0f);
			Renderer::getInstancePtr()->updatePerViewData(camera);

			if (m_environmentTexture)
			{
				m_diffuseReflectionShader.bind();

				Renderer::getInstancePtr()->setPerFrameBuffersForVS();
				Renderer::getInstancePtr()->setPerViewBuffersForVS();

				m_environmentTexture->bindSRVForPS(20);
				Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

				m_environmentTextureInfoCBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 10);

				D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
			}
		}

		save(textureFileName, diffuseTexture, false, FileFormat::BC6_UNSIGNED);
	}
	void ReflectionCapture::captureSpecularReflection(std::wstring specularTextureFileName, std::wstring factorTextureFileName)
	{
		captureSpecularIrradianceReflection(specularTextureFileName);
		captureSpecularFactorReflection(factorTextureFileName);
	}
	void ReflectionCapture::captureSpecularIrradianceReflection(std::wstring specularIrradianceTextureFileName)
	{
		Camera camera;
		camera.setPerspective(90.0f, 1.0f, 0.1f, 10.0f);

		math::Angles cameraAngles[6] = {
			math::Angles({0.0f, math::deg2rad(-90.0f), 0.0f}),
			math::Angles({0.0f, math::deg2rad(90.0f), 0.0f}),
			math::Angles({math::deg2rad(90.0f), 0.0f, 0.0f}),
			math::Angles({math::deg2rad(-90.0f), 0.0f, 0.0f}),
			math::Angles({0.0f, 0.0f ,0.0f}),
			math::Angles({0.0f, math::deg2rad(180.0f), 0.0f})
		};

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = 1;

		D3D11_TEXTURE2D_DESC specTexDesc{};
		specTexDesc.Width = 1024;
		specTexDesc.Height = 1024;
		specTexDesc.MipLevels = 0;
		specTexDesc.ArraySize = 6;
		specTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		specTexDesc.SampleDesc.Count = 1;
		specTexDesc.SampleDesc.Quality = 0;
		specTexDesc.Usage = D3D11_USAGE_DEFAULT;
		specTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
		specTexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		DxResPtr<ID3D11Texture2D> specularTexture;
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&specTexDesc, nullptr, specularTexture.reset());

		struct RoughnessBuffer
		{
			float roughness;
			math::Vec3f pad;
		};
		Buffer<RoughnessBuffer> roughnessBuffer;
		auto device = D3D::getInstancePtr()->getDevice();
		roughnessBuffer.createConstantBuffer(device);
		for (int i = 0; i < 6; i++)
		{
			for (int j = 0; j < 11; j++)
			{
				rtvDesc.Texture2DArray.FirstArraySlice = i;
				rtvDesc.Texture2D.MipSlice = j;

				DxResPtr<ID3D11RenderTargetView> rtv;
				auto result = D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(specularTexture, &rtvDesc, rtv.reset());

				Renderer::getInstancePtr()->setRenderTarget(rtv, {});

				int w = specTexDesc.Width >> j;
				int h = specTexDesc.Height >> j;

				D3D11_VIEWPORT viewport = {};
				viewport.TopLeftX = 0;
				viewport.TopLeftY = 0;
				viewport.Width = w;
				viewport.Height = h;
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;

				Renderer::getInstancePtr()->setViewport(viewport);

				//render
				camera.setWorldAngles(cameraAngles[i]);
				camera.updateCamera();

				Renderer::getInstancePtr()->updatePerFrameData(camera, camera, 0.0f, 0.0f);
				Renderer::getInstancePtr()->updatePerViewData(camera);

				m_environmentTextureInfoCBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 10);

				auto res = roughnessBuffer.map(D3D::getInstancePtr()->getDeviceContext());
				RoughnessBuffer* roughness = static_cast<RoughnessBuffer*>(res.pData);
				roughness->roughness = j * 0.1f;
				roughnessBuffer.unmap(D3D::getInstancePtr()->getDeviceContext());
				roughnessBuffer.setConstantBufferForPixelShader(D3D::getInstancePtr()->getDeviceContext(), 11);

				if (m_environmentTexture)
				{
					m_specularReflectionShader.bind();

					Renderer::getInstancePtr()->setPerFrameBuffersForVS();
					Renderer::getInstancePtr()->setPerViewBuffersForVS();

					m_environmentTexture->bindSRVForPS(20);
					Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

					D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
					D3D::getInstancePtr()->getDeviceContext()->Flush();
				}
			}
		}

		save(specularIrradianceTextureFileName, specularTexture, false, FileFormat::BC6_UNSIGNED);
	}
	void ReflectionCapture::captureSpecularFactorReflection(std::wstring specularFactorTextureFileName)
	{
		D3D11_TEXTURE2D_DESC factorTexDesc{};
		factorTexDesc.Width = 1024;
		factorTexDesc.Height = 1024;
		factorTexDesc.MipLevels = 1;
		factorTexDesc.ArraySize = 1;
		factorTexDesc.Format = DXGI_FORMAT_R8G8_UNORM;
		factorTexDesc.SampleDesc.Count = 1;
		factorTexDesc.SampleDesc.Quality = 0;
		factorTexDesc.Usage = D3D11_USAGE_DEFAULT;
		factorTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

		DxResPtr<ID3D11Texture2D> factorTexture;
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&factorTexDesc, nullptr, factorTexture.reset());

		DxResPtr<ID3D11RenderTargetView> rtv;
		auto result = D3D::getInstancePtr()->getDevice()->CreateRenderTargetView(factorTexture, nullptr, rtv.reset());

		Renderer::getInstancePtr()->setRenderTarget(rtv, {});

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = factorTexDesc.Width;
		viewport.Height = factorTexDesc.Height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		Renderer::getInstancePtr()->setViewport(viewport);

		//render
		m_reflectanceFactorShader.bind();

		Renderer::getInstancePtr()->setPerFrameBuffersForVS();
		Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

		D3D::getInstancePtr()->getDeviceContext()->Draw(3, 0);
		D3D::getInstancePtr()->getDeviceContext()->Flush();

		save(specularFactorTextureFileName, factorTexture, false, FileFormat::BC5_UNSIGNED);
	}
}