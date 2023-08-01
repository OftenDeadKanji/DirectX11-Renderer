#pragma once
#include "../render/Direct3d/d3d.h"
#include "../math/mathUtils.h"
#include "../utils/nonCopyable.h"
#include "../render/Direct3d/buffer.h"
#include "../render/skyRenderer/skyRenderer.h"
#include "../render/postProcess/postProcess.h"
#include "../render/reflectionCapture/reflectionCapture.h"
#include "../render/fogRenderer/fogRenderer.h"

#define SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE 1
#define MAX_GPU_PARTICLES 10000

namespace Engine
{
	class Camera;
	class Shader;

	class Renderer
		: public NonCopyable
	{
	public:
		struct GBuffer
		{
			DxResPtr<ID3D11Texture2D> albedo;
			DxResPtr<ID3D11RenderTargetView> albedoRTV;
			DxResPtr<ID3D11ShaderResourceView> albedoSRV;

			DxResPtr<ID3D11Texture2D> roughness_metalness;
			DxResPtr<ID3D11RenderTargetView> roughness_metalnessRTV;
			DxResPtr<ID3D11ShaderResourceView> roughness_metalnessSRV;

			DxResPtr<ID3D11Texture2D> normal;
			DxResPtr<ID3D11RenderTargetView> normalRTV;
			DxResPtr<ID3D11ShaderResourceView> normalSRV;

			DxResPtr<ID3D11Texture2D> emission;
			DxResPtr<ID3D11RenderTargetView> emissionRTV;
			DxResPtr<ID3D11ShaderResourceView> emissionSRV;

			DxResPtr<ID3D11Texture2D> objectID;
			DxResPtr<ID3D11RenderTargetView> objectIDRTV;
			DxResPtr<ID3D11ShaderResourceView> objectIDSRV;

			DxResPtr<ID3D11Texture2D> depthStencil;
			DxResPtr<ID3D11DepthStencilView> m_depthStencilViewReadWrite;
			DxResPtr<ID3D11DepthStencilView> m_depthStencilViewReadOnly;
			DxResPtr<ID3D11ShaderResourceView> depthSRV;

			static constexpr int BUFFERS_COUNT = 5;
		};
	public:
		static Renderer* createInstance();
		static void deleteInstance();

		static Renderer* getInstancePtr();

		void init(DxResPtr<ID3D11Texture2D> backbufferTexture);
		void initDepthRendering();
		void initDepth2DRendering();
		void initDepthCubemapRendering(bool first = false);
		void deinit();

		void updatePerFrameData(const Camera& camera, const Camera& prevCamera, float deltaTime, float timeSinceStart);
		void updatePerViewData(const Camera& camera, bool useCameraCenteredWSdataIfSet = 1);

		void update(const Camera& camera, const Camera& prevCamera, float deltaTime, float timeSinceStart);
		void render(Camera& camera);

		void setPerFrameBuffersForVS();
		void setPerFrameBuffersForGS();
		void setPerFrameBuffersForPS();
		void setPerFrameBuffersForCS();

		void setPerViewBuffersForVS();
		void setPerViewBuffersForGS();
		void setPerViewBuffersForPS();
		void setPerViewBuffersForCS();

		void setPerFrameGlobalSamplersForPS();
		void setPerFrameGlobalSamplersForCS();

		void setSceneGBufferDepthSRVForPS(int slot);
		void unsetSceneDepthSRVForPS(int slot);

		void setSceneGBufferDepthSRVForCS(int slot);
		void unsetSceneDepthSRVForCS(int slot);

		void updateBackbuffer(DxResPtr<ID3D11Texture2D> backbufferTexture);
		void updateGBuffer();

		void releaseBackbufferResources();

		void setClearColor(const math::Vec3f& color);
		void setSkyTexture(std::shared_ptr<Texture> texture);
		void setIBLTextures(std::shared_ptr<Texture> diffuseIBL, std::shared_ptr<Texture> specularIBL, std::shared_ptr<Texture> factorIBL);
		void setEV100(float EV100);
		void setFXAAQualitySubpix(float quality);
		void setFXAAQualityEdgeThreshhold(float quality);
		void setFXAAQualityEdgeThreshholdMin(float quality);

		float getFXAAQualitySubpix() const;
		float getFXAAQualityEdgeThreshhold() const;
		float getFXAAQualityEdgeThreshholdMin() const;

		void prerenderIBLTextures(const std::wstring& diffuseTexFilePath, const std::wstring& specularTexFilePath, const std::wstring& reflectanceTexFilePath);
		void setRenderTarget(DxResPtr<ID3D11RenderTargetView> renderTagetView, DxResPtr<ID3D11DepthStencilView> depthStencilView);
		void setViewport(D3D11_VIEWPORT viewport);
		void adjustViewportToBackbuffer();

		void setShouldUseDiffuseReflection(bool shouldUse);
		void setShouldUseSpecularReflection(bool shouldUse);
		void setShouldUseIBL(bool shouldUse);
		void setShouldOverwriteRoughness(bool shouldOverwrite);
		void setOverwrittenRoughnessValue(float roughness);

		bool shouldUseDiffuseReflection() const;
		bool shouldUseSpecularReflection() const;
		bool shouldUseIBL() const;
		bool shouldOverwriteRoughness() const;
		float getOverwrittenRoughnessValue() const;

		int getDirectionalLightShadowResolution() const;
		int getPointLightShadowResolution() const;
		int getSpotLightShadowResolution() const;

		void setDirectionalLightShadowResolution(int resolution);
		void setPointLightShadowResolution(int resolution);
		void setSpotLightShadowResolution(int resolution);

		void enableBlending();
		void enableAlphaToCoverage();
		void enableAdditiveBlending();
		void disableBlending();

		void switchToGBufferBlending();
		void switchToGBufferBlendingWithAlphaToCoverage();

		void switchToDepthEnabledStencilDisabledState();
		void setReadWriteStencilRefValue(unsigned int value);
		void setReadOnlyStencilRefValue(unsigned int value);

		void setFrontFaceCulling();
		void setBackFaceCulling();

		GBuffer& getGBuffer();

		void setFogDensity(float density);
		float getFogDensity();
		void setFogPhaseFunctionParameter(float phase);
		float getFogPhaseFunctionParameter();

		void enableUniformFog();
		bool isUniformFogEnabled() const;

		void enableVolumetricFog();
		bool isVolumetricFogEnabled() const;

		void disableFog();
		bool isFogEnabled() const;

		void bindVolumetricFogTextureForPS(int slot);
	private:
		Renderer() = default;
		static Renderer* s_instance;

		void createBackbuffer(DxResPtr<ID3D11Texture2D> backbufferTexture);
		void initDepthStencilBuffer(DxResPtr<ID3D11Texture2D> backbufferTexture);
		void initSamplers();
		void initBlendState();
		void initGBuffer();

		void clearViews();
		void renderDepth(Camera& camera);
		void renderStencil(Camera& camera);
		void renderGBufferGeometry(Camera& camera);
		void renderDecals(Camera& camera);
		void renderGBufferLighing(Camera& camera);
		void renderMeshes(Camera& camera);
		void renderUniformFog();
		void renderVolumetricFog(Camera& camera);
		void renderSkybox(Camera& camera);
		void renderParticleSystem(Camera& camera);
		void convertHDRtoLDR();
		void applyAA();
		void applyBloom();

		math::Vec3f m_clearColor;

		math::Vec2i m_backbufferResolution;

		DxResPtr<ID3D11Texture2D> m_depthStencilBuffer;
		DxResPtr<ID3D11DepthStencilState> m_DSSDeptEnabledStencilDisabled;
		DxResPtr<ID3D11DepthStencilState> m_DSSDeptEnabledStencilReadWrite;
		DxResPtr<ID3D11DepthStencilState> m_DSSDeptEnabledStencilReadOnly;
		DxResPtr<ID3D11DepthStencilState> m_DSSDeptDisabledStencilReadOnly;
		DxResPtr<ID3D11DepthStencilView> m_depthStencilViewReadWrite;
		DxResPtr<ID3D11DepthStencilView> m_depthStencilViewReadOnly;
		DxResPtr<ID3D11ShaderResourceView> m_depthSRV;

		DxResPtr<ID3D11RasterizerState> m_rasterizerState;
		DxResPtr<ID3D11RasterizerState> m_rasterizerStateFrontCulling;

		DxResPtr<ID3D11Texture2D> m_backbufferTexture;
		DxResPtr<ID3D11RenderTargetView> m_backbufferRTV;

		DxResPtr<ID3D11Texture2D> m_ldrTexture;
		DxResPtr<ID3D11RenderTargetView> m_ldrRTV;

		DxResPtr<ID3D11Texture2D> m_hdrTexture;
		DxResPtr<ID3D11RenderTargetView> m_hdrRTV;

		std::shared_ptr<Texture> m_diffuseIBLTexture;
		std::shared_ptr<Texture> m_specularIBLTexture;
		std::shared_ptr<Texture> m_factorIBLTexture;

		struct PerFrame
		{
			float timeSinceStart;
			float deltaTime;
			uint32_t useDiffuseReflection;
			uint32_t useSpecularReflection;

			uint32_t useIBL;
			uint32_t useRoughnessOverwriting;
			float overwrittenRoughness;
			uint32_t specularIBLTextureMipLevels;
		};
		Buffer<PerFrame> perFrameConstantBuffer;

		struct GBufferInfo
		{
			math::Vec2i gbufferTexturesSize;
			math::Vec2i pad;
		};
		Buffer<GBufferInfo> gbufferConstantBuffer;

		struct PerView
		{
			math::Mat4f view;
			math::Mat4f viewInv;
			math::Mat4f proj;
			math::Mat4f projInv;
			math::Mat4f viewProj;
			math::Mat4f viewProjInv;

			math::Vec3f cameraPosition;
			float cameraZNear;

			math::Vec3f camFrustumBL;
			float cameraZFar;

			math::Vec3f camFrustumTL;
			float pad2;

			math::Vec3f camFrustumBR;
			float pad3;
		};
		Buffer<PerView> perViewConstantBuffer;

		//global samplers
		DxResPtr<ID3D11SamplerState> m_samplerPointWrap;
		DxResPtr<ID3D11SamplerState> m_samplerBilinearWrap;
		DxResPtr<ID3D11SamplerState> m_samplerTrilinearWrap;
		DxResPtr<ID3D11SamplerState> m_samplerAnisotropicWrap;
		DxResPtr<ID3D11SamplerState> m_samplerDepth;
		DxResPtr<ID3D11SamplerState> m_samplerBilinearClamp;

		SkyRenderer m_skyRenderer;

		PostProcess postProcess;
		float m_gamma, m_ev100;
		float fxaaQualitySubpix, fxaaQualityEdgeThreshold, fxaaQualityEdgeThresholdMin;

		bool useDiffuseReflection = true;
		bool useSpecularReflection = true;
		bool useIBL = true;
		bool useRoughnessOverwriting = false;
		float overwrittenRoughness = 0.0f;

		int m_directionalLightResolution = 2048;
		int m_pointLightResolution = 1024;
		int m_spotLightResolution = 1024;

		DxResPtr<ID3D11RasterizerState> m_depthRasterizerState;

		DxResPtr<ID3D11RenderTargetView> depthDirLightRTV;
		DxResPtr<ID3D11DepthStencilState> depthDirLightDSState;
		DxResPtr<ID3D11DepthStencilView> depthDirLightDSV;
		DxResPtr<ID3D11Texture2D> depthDirLightTexture;
		DxResPtr<ID3D11ShaderResourceView> depthDirLightSRV;

		DxResPtr<ID3D11RenderTargetView> depthSpotLightRTV;
		DxResPtr<ID3D11DepthStencilState> depthSpotLightDSState;
		DxResPtr<ID3D11DepthStencilView> depthSpotLightDSV;
		DxResPtr<ID3D11Texture2D> depthSpotLightTexture;
		DxResPtr<ID3D11ShaderResourceView> depthSpotLightSRV;

		int depthCubemapsCount = 1;
		DxResPtr<ID3D11RenderTargetView> depthCubemapRTV;
		DxResPtr<ID3D11DepthStencilState> depthCubemapDSState;
		DxResPtr<ID3D11DepthStencilView> depthCubemapDSV;
		DxResPtr<ID3D11Texture2D> depthCubemapTexture;
		DxResPtr<ID3D11ShaderResourceView> depthCubemapSRV;

		DxResPtr<ID3D11BlendState1> m_blendStateBlendingEnabled;
		DxResPtr<ID3D11BlendState1> m_blendStateAlphaToCoverageEnabled;
		DxResPtr<ID3D11BlendState1> m_blendStateAdditiveBlendingEnabled;
		
		DxResPtr<ID3D11BlendState1> m_blendStateGBuffer;
		DxResPtr<ID3D11BlendState1> m_blendStateGBufferAlphaToCoverage;

		GBuffer m_GBuffer;

		Shader m_gbufferPBRLightingShader;
		Shader m_gbufferNoLightingShader;

		FogRenderer m_fogRenderer;
	};
}
