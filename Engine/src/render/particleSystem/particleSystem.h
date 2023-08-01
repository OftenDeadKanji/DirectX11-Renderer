#pragma once
#include "particleEmitter.h"
#include "../../utils/nonCopyable.h"
#include "../shader/shader.h"
#include "../Direct3d/buffer.h"
#include "../camera/camera.h"
#include "../texture/texture.h"

namespace Engine
{
	class ParticleSystem
		: public NonCopyable
	{
	public:
		struct ParticleRingBuffer
		{
			struct ParticleInstance
			{
				math::Vec4f color;
				math::Vec3f position;
				float lifetime;
				math::Vec3f speed;
				float size;
			};
			Buffer<ParticleInstance> m_gpuParticlesBuffer;

			struct ParticleDataRange
			{
				unsigned int number;
				unsigned int offset;
				unsigned int expired;

				unsigned int billboardIndexCountPerInstance;
				unsigned int billboardInstanceCount;
				unsigned int billboardStartIndexLocation;
				unsigned int billboardBaseVertexLocation;
				unsigned int billboardStartInstanceLocation;

				unsigned int sphereIndexCountPerInstance;
				unsigned int sphereInstanceCount;
				unsigned int sphereStartIndexLocation;
				unsigned int sphereBaseVertexLocation;
				unsigned int sphereStartInstanceLocation;
			};
			Buffer<ParticleDataRange> m_rangeBuffer;
		};

	public:
		static ParticleSystem* createInstance();
		static ParticleSystem* getInstance();
		static void deleteInstance();

		void init();

		void addSmokeEmitter(TransformSystem::ID transformID, float emitterSpawnRate, float emitterSpawnRadius, const math::Vec3f& emitterParticlesColor);
		void setSmokeTexture(std::shared_ptr<Texture> smokeRLTTexture, std::shared_ptr<Texture> smokeBBFTexture, std::shared_ptr<Texture> smokeEMVATexture);

		void updateCPUParticles(float deltaTime);
		void updateGPUParticles();
		void render(Camera& camera);
		void renderParticles(Camera& camera);
		void renderGPUParticlesLights();

		ParticleRingBuffer& getParticleRingBuffer();
	private:
		void initCPUParticles();
		void initGPUParticles();

		//void renderGPUParticlesLights();
		void renderGPUParticles();

		void renderCPUParticles(Camera& camera);

		static ParticleSystem* s_instance;

		std::vector<ParticleEmmiter> m_smokeEmitters;
		std::shared_ptr<Texture> m_smokeRLTTexture;
		std::shared_ptr<Texture> m_smokeBBFTexture;
		std::shared_ptr<Texture> m_smokeEMVATexture;
		math::Vec2i atlasSize = { 8, 8 };

		struct AtlasCBuffer
		{
			math::Vec2i atlasSize;
			math::Vec2i pad;
		};
		Buffer<AtlasCBuffer> m_atlasCBuffer;

		Shader shader;
		Buffer<unsigned int> m_billboardIndices;

		struct ParticleInstance
		{
			math::Vec3f position;
			math::Vec4f color;
			float rotationAgle;
			math::Vec2f size;
			int frameIndex;
			float frameFraction;
		};
		Buffer<ParticleInstance> m_instanceBuffer;

		ParticleRingBuffer m_particleRingBuffer;
		Shader m_gpuParticlesDataUpdateShader;
		Shader m_gpuParticlesRangeUpdateShader;
		Shader m_gpuParticleRenderShader;
		std::shared_ptr<Texture> m_gpuParticleTexture;
		
		Shader m_gpuParticlesSphereRenderShader;
	};
}