#pragma once
#include "../Direct3d/d3d.h"
#include "../shader/shader.h"
#include "../Direct3d/buffer.h"

namespace Engine
{
	class PostProcess
	{
	public:
		PostProcess() = default;

		void init();
		void resolve(DxResPtr<ID3D11RenderTargetView> src, DxResPtr<ID3D11RenderTargetView> dst);
		void applyFXAA(DxResPtr<ID3D11RenderTargetView> src, DxResPtr<ID3D11RenderTargetView> dst);
		void applyBloom(const DxResPtr<ID3D11RenderTargetView>& src, DxResPtr<ID3D11RenderTargetView>& dst, int bloomChainLength);

		void setGamma(float gamma)
		{
			m_hdr2ldrParams.gamma = gamma;
		}

		void setEV100(float ev100)
		{
			m_hdr2ldrParams.ev100 = ev100;
		}

		void setFXAAQualitySubpix(float quality)
		{
			m_fxaaParams.qualitySubpix = quality;
		}
		void setFXAAEdgeThreshhold(float quality)
		{
			m_fxaaParams.qualityEdgeThreshold = quality;
		}
		void setFXAAEdgeThreshholdMin(float quality)
		{
			m_fxaaParams.qualityEdgeThresholdMin = quality;
		}
	private:
		void initBloomChain(const DxResPtr<ID3D11RenderTargetView>& src, int bloomChainLength);
		void bloomDownsample(const DxResPtr<ID3D11RenderTargetView>& src, int bloomChainLength);
		void bloomUpsample(DxResPtr<ID3D11RenderTargetView>& dst, int bloomChainLength);

		DxResPtr<ID3D11ShaderResourceView> m_srv;
		
		Shader m_hdr2ldrShader;
		struct Hdr2LdrParams
		{
			float gamma;
			float ev100;
			
			int64_t padding;
		} m_hdr2ldrParams;
		Buffer<Hdr2LdrParams> m_hdr2ldrParamsCBuffer;

		Shader m_fxaaShader;
		struct FxaaParams
		{
			math::Vec4f imageSize;
			float qualitySubpix;
			float qualityEdgeThreshold;
			float qualityEdgeThresholdMin;
			float pad;
		} m_fxaaParams;
		Buffer<FxaaParams> m_fxaaParamsCBuffer;

		struct BloomChain
		{
			std::vector<DxResPtr<ID3D11Texture2D>> textures;
			std::vector<math::Vec2i> sizes;
		} m_bloomChain;

		Shader m_bloomDownsampleShader;
		struct BloomDownsampleInfo
		{
			math::Vec2i textureSize;
			float pad[2];
		} m_bloomDownsampleInfo;
		Buffer<BloomDownsampleInfo> m_bloomDownsampleCBuffer;

		Shader m_bloomUpsampleShader;
		struct BloomUpsampleInfo
		{
			float filterRadius;
			float pad[3];
		} m_bloomUpsampleInfo;
		Buffer<BloomUpsampleInfo> m_bloomUpsampleCBuffer;

		Shader m_bloomFinalShader;
		DxResPtr<ID3D11RenderTargetView> m_bloomRTV;
	};
}