#include "particleSystem.h"
#include <array>
#include "../../engine/renderer.h"
#include "../../utils/sorting/radixSort.h"
#include "../../resourcesManagers/modelManager.h"
#include "../../resourcesManagers/textureManager.h"

namespace Engine
{
	ParticleSystem* ParticleSystem::s_instance = nullptr;

	ParticleSystem* ParticleSystem::createInstance()
	{
		return s_instance = new ParticleSystem();
	}
	ParticleSystem* ParticleSystem::getInstance()
	{
		return s_instance;
	}
	void ParticleSystem::deleteInstance()
	{
		delete s_instance;
		s_instance = nullptr;
	}
	void ParticleSystem::init()
	{
		initCPUParticles();
		initGPUParticles();
	}
	void ParticleSystem::addSmokeEmitter(TransformSystem::ID transformID, float emitterSpawnRate, float emitterSpawnRadius, const math::Vec3f& emitterParticlesColor)
	{
		m_smokeEmitters.emplace_back(transformID, emitterSpawnRate, emitterSpawnRadius, emitterParticlesColor);
	}
	void ParticleSystem::setSmokeTexture(std::shared_ptr<Texture> smokeRLTTexture, std::shared_ptr<Texture> smokeBBFTexture, std::shared_ptr<Texture> smokeEMVATexture)
	{
		m_smokeRLTTexture = smokeRLTTexture;
		m_smokeBBFTexture = smokeBBFTexture;
		m_smokeEMVATexture = smokeEMVATexture;
	}
	void ParticleSystem::updateCPUParticles(float deltaTime)
	{
		for (auto& emitter : m_smokeEmitters)
		{
			emitter.update(deltaTime);
		}

	}
	void ParticleSystem::updateGPUParticles()
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		ID3D11ShaderResourceView* srv[] = { renderer->getGBuffer().depthSRV.ptr() };
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);

		m_particleRingBuffer.m_gpuParticlesBuffer.setUAVForCS(devcon, 5);
		m_particleRingBuffer.m_rangeBuffer.setUAVForCS(devcon, 6);

		{
			m_gpuParticlesDataUpdateShader.bind();

			renderer->setPerFrameBuffersForCS();
			renderer->setPerViewBuffersForCS();

			renderer->setPerFrameGlobalSamplersForCS();

			ID3D11ShaderResourceView* SRVs[] = {
				renderer->getGBuffer().normalSRV,
				renderer->getGBuffer().objectIDSRV
			};
			devcon->CSSetShaderResources(21, 2, SRVs);

			devcon->Dispatch(MAX_GPU_PARTICLES, 1, 1);

			SRVs[0] = SRVs[1] = NULL;
			devcon->CSSetShaderResources(21, 2, SRVs);
		}
		{
			m_gpuParticlesRangeUpdateShader.bind();

			devcon->Dispatch(1, 1, 1);
		}

		m_particleRingBuffer.m_gpuParticlesBuffer.unsetUAVForCS(devcon, 5);
		m_particleRingBuffer.m_rangeBuffer.unsetUAVForCS(devcon, 6);

		srv[0] = NULL;
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);
	}

	void ParticleSystem::render(Camera& camera)
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		ID3D11ShaderResourceView* srv[] = { renderer->getGBuffer().depthSRV.ptr() };
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);

		//updateGPUParticles();
		renderGPUParticlesLights();
		renderGPUParticles();

		renderCPUParticles(camera);

		srv[0] = NULL;
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);
	}
	void ParticleSystem::renderParticles(Camera& camera)
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		ID3D11ShaderResourceView* srv[] = { renderer->getGBuffer().depthSRV.ptr() };
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);

		renderGPUParticles();
		renderCPUParticles(camera);

		srv[0] = NULL;
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);
	}
	void ParticleSystem::renderGPUParticlesLights()
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		ID3D11ShaderResourceView* srv[] = { renderer->getGBuffer().depthSRV.ptr() };
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);

		renderer->enableAdditiveBlending();
		m_gpuParticlesSphereRenderShader.bind();

		auto sphereModel = ModelManager::getInstancePtr()->getUnitSphereModel();
		sphereModel->setVertexBufferForIA();
		sphereModel->setIndexBufferForIA();

		m_particleRingBuffer.m_gpuParticlesBuffer.setBufferForVS(devcon, 20);
		m_particleRingBuffer.m_rangeBuffer.setBufferForVS(devcon, 21);

		ID3D11ShaderResourceView* gbufferSRV[] = {
			renderer->getGBuffer().albedoSRV,
			renderer->getGBuffer().roughness_metalnessSRV,
			renderer->getGBuffer().normalSRV
		};

		devcon->PSSetShaderResources(20, 3, gbufferSRV);

		devcon->DrawIndexedInstancedIndirect(m_particleRingBuffer.m_rangeBuffer.getBufferPtr(), 8 * sizeof(unsigned int));

		gbufferSRV[0] = gbufferSRV[1] = gbufferSRV[2] = NULL;
		devcon->PSSetShaderResources(20, 3, gbufferSRV);

		m_particleRingBuffer.m_gpuParticlesBuffer.unsetBufferForVS(devcon, 20);
		m_particleRingBuffer.m_rangeBuffer.unsetBufferForVS(devcon, 21);

		renderer->disableBlending();

		srv[0] = NULL;
		devcon->PSSetShaderResources(23, 1, srv);
		devcon->CSSetShaderResources(23, 1, srv);
	}
	ParticleSystem::ParticleRingBuffer& ParticleSystem::getParticleRingBuffer()
	{
		return m_particleRingBuffer;
	}
	void ParticleSystem::initCPUParticles()
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc =
		{
			{"POS",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"COL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"ANG",		0, DXGI_FORMAT_R32_FLOAT,			0, 32,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"SIZE",	0, DXGI_FORMAT_R32G32_FLOAT,		0, 36,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"FIDX",	0, DXGI_FORMAT_R32_UINT,			0, 44,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"FFRAC",	0, DXGI_FORMAT_R32_FLOAT,			0, 48,	D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		shader.init(L"Shaders/particle/particleVS.hlsl", L"Shaders/particle/particlePS.hlsl", inputElementDesc);

		unsigned int indices[6] = { 0, 1, 2, 1, 3, 2 };
		m_billboardIndices.createIndexBuffer(6, indices, D3D::getInstancePtr()->getDevice());
		m_atlasCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
	}
	void ParticleSystem::initGPUParticles()
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc =
		{
			{"POS",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COL",			0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEX",			0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORM",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BTANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		m_gpuParticlesSphereRenderShader.init(L"Shaders/particle/gpuParticleSphereVS.hlsl", L"Shaders/particle/gpuParticleSpherePS.hlsl", inputElementDesc);

		m_gpuParticlesDataUpdateShader.initComputeShader(L"Shaders/particle/gpuParticleDataUpdateCS.hlsl");
		m_gpuParticlesRangeUpdateShader.initComputeShader(L"Shaders/particle/gpuParticleRangeUpdateCS.hlsl");
		m_gpuParticleRenderShader.init(L"Shaders/particle/gpuParticleVS.hlsl", L"Shaders/particle/gpuParticlePS.hlsl", {});

		m_particleRingBuffer.m_gpuParticlesBuffer.createStructuredRWBuffer(MAX_GPU_PARTICLES, D3D::getInstancePtr()->getDevice());

		ParticleRingBuffer::ParticleDataRange initData{};
		initData.billboardIndexCountPerInstance = 6;
		initData.sphereIndexCountPerInstance = ModelManager::getInstancePtr()->getUnitSphereModel()->getMeshRange(0).indexNum;
		m_particleRingBuffer.m_rangeBuffer.createRWBuffer(&initData, 13, DXGI_FORMAT_R32_UINT, D3D::getInstancePtr()->getDevice(), D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS);

		m_gpuParticleTexture = TextureManager::getInstance()->getTexture(L"Assets/Textures/2D/spark5.dds");
	}

	void ParticleSystem::renderGPUParticles()
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		renderer->enableBlending();
		m_gpuParticleRenderShader.bind();

		renderer->setPerFrameBuffersForVS();
		renderer->setPerFrameBuffersForPS();

		renderer->setPerViewBuffersForVS();
		renderer->setPerViewBuffersForPS();

		renderer->setPerFrameGlobalSamplersForPS();

		m_particleRingBuffer.m_gpuParticlesBuffer.setBufferForVS(devcon, 20);
		m_particleRingBuffer.m_rangeBuffer.setBufferForVS(devcon, 21);

		m_gpuParticleTexture->bindSRVForPS(20);
		renderer->bindVolumetricFogTextureForPS(24);

		m_billboardIndices.setIndexBufferForInputAssembler(devcon);

		devcon->DrawIndexedInstancedIndirect(m_particleRingBuffer.m_rangeBuffer.getBufferPtr(), 3 * sizeof(unsigned int));

		m_particleRingBuffer.m_gpuParticlesBuffer.unsetBufferForVS(devcon, 20);
		m_particleRingBuffer.m_rangeBuffer.unsetBufferForVS(devcon, 21);
	}

	void ParticleSystem::renderCPUParticles(Camera& camera)
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto* renderer = Renderer::getInstancePtr();

		int smokeFramesCount = atlasSize.x() * atlasSize.y();

		std::vector<ParticleInstance> particles;
		for (const auto& emitter : m_smokeEmitters)
		{
			for (const auto& particle : emitter.getParticles())
			{
				ParticleInstance inst;

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
				inst.position = particle.position - camera.position();
#else
				inst.position = particle.position;
#endif

				inst.color = particle.color;
				inst.rotationAgle = particle.rotationAngle;
				inst.size = particle.size;

				float frame = smokeFramesCount * (particle.lifetime / particle.maxLifeTime);
				inst.frameIndex = static_cast<int>(frame);
				inst.frameFraction = frame - inst.frameIndex;

				particles.push_back(inst);
			}
		}
		particles.shrink_to_fit();

		if (particles.size() > 1)
		{
			std::vector<float> fVecIn(particles.size()), fVecOut(particles.size());
			for (int i = 0; i < particles.size(); i++)
			{
				fVecIn[i] = (camera.position() - particles[i].position).norm();
			}

			std::vector<ParticleInstance> particlesCopy(particles.size());

			RadixSort11(fVecIn.data(), fVecOut.data(), particles, particlesCopy);
			std::reverse(particlesCopy.begin(), particlesCopy.end());

			particles = std::move(particlesCopy);
		}

		if (!particles.empty())
		{
			m_instanceBuffer.createInstanceBuffer(particles.size(), nullptr, D3D::getInstancePtr()->getDevice());

			shader.bind();
			renderer->setPerFrameBuffersForVS();
			renderer->setPerFrameBuffersForPS();

			renderer->setPerViewBuffersForVS();
			renderer->setPerViewBuffersForPS();

			renderer->setPerFrameGlobalSamplersForPS();
			{
				auto atlasMap = m_atlasCBuffer.map(devcon);

				static_cast<AtlasCBuffer*>(atlasMap.pData)->atlasSize = atlasSize;

				m_atlasCBuffer.unmap(devcon);
				m_atlasCBuffer.setConstantBufferForPixelShader(devcon, 10);
			}

			m_smokeRLTTexture->bindSRVForPS(20);
			m_smokeBBFTexture->bindSRVForPS(21);
			m_smokeEMVATexture->bindSRVForPS(22);
			renderer->bindVolumetricFogTextureForPS(24);

			auto mapped = m_instanceBuffer.map(devcon);
			ParticleInstance* inst = static_cast<ParticleInstance*>(mapped.pData);
			for (int i = 0; i < particles.size(); i++)
			{
				inst[i] = particles[i];
			}
			m_instanceBuffer.unmap(devcon);

			m_billboardIndices.setIndexBufferForInputAssembler(devcon);
			m_instanceBuffer.setInstanceBufferForInputAssembler(devcon, 0);

			devcon->DrawIndexedInstanced(6, particles.size(), 0, 0, 0);
		}
	}
}