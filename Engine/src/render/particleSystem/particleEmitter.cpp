#include "particleEmitter.h"
#include "../../utils/random/random.h"
#include <numbers>

namespace Engine
{
	ParticleEmmiter::ParticleEmmiter(TransformSystem::ID transformID, float spawnRate, float spawnRadius, const math::Vec3f& particlesColor)
		: m_transformID(transformID), m_spawnRate(spawnRate), m_spawnRadius(spawnRadius), m_particlesColor(particlesColor), m_time(0.0f)
	{
		m_verticalSpeed = Random::getInstance()->getRandomFloat();
	}

	void ParticleEmmiter::update(float deltaTime)
	{
		for (auto iter = m_particles.begin(); iter != m_particles.end();)
		{
			iter->lifetime += deltaTime;

			if (iter->lifetime >= iter->maxLifeTime)
			{
				iter = m_particles.erase(iter);
			}
			else
			{
				//update alpha value (spawn/despawn)
				if (iter->lifetime < m_spawnTime)
				{
					iter->color.w() = std::lerp(0.0, 1.0, iter->lifetime / m_spawnTime);
				}
				else if (iter->lifetime > iter->maxLifeTime - m_despawnTime)
				{
					iter->color.w() = std::lerp(1.0, 0.0, (iter->lifetime - (iter->maxLifeTime - m_despawnTime)) / m_despawnTime);
				}

				//update position
				iter->position += iter->speed * deltaTime;
				iter->size += math::Vec2f(m_sizeIncreaseSpeed, m_sizeIncreaseSpeed) * deltaTime;

				iter++;
			}
		}

		m_time += deltaTime;

		int particlesToSpawnCount = m_time * m_spawnRate;
		spawnParticles(particlesToSpawnCount);

		m_time -= particlesToSpawnCount * (1.0 / m_spawnRate);
	}

	const std::vector<Particle>& ParticleEmmiter::getParticles() const
	{
		return m_particles;
	}

	void ParticleEmmiter::spawnParticles(int count)
	{
		auto* random = Random::getInstance();
		auto* transformSystem = TransformSystem::getInstance();
		
		math::Mat4f& transformMatrix = transformSystem->getMatrix(m_transformID);
		math::Vec3f emitterPosition = math::getTranslation(transformMatrix);

		for (int i = 0; i < count; i++)
		{
			Particle particle;

			particle.color = math::Vec4f(m_particlesColor.x(), m_particlesColor.y(), m_particlesColor.z(), 0.0f);
			
			particle.position = emitterPosition + random->getRandomVec3f(0.0f, m_spawnRadius);
			particle.position.y() = emitterPosition.y();

			particle.rotationAngle = random->getRandomFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
			particle.size = m_initialSize;

			particle.speed = random->getRandomVec3f(0.1f, 0.2f);
			particle.speed.y() = m_verticalSpeed;

			particle.lifetime = 0.0f;
			particle.maxLifeTime = random->getRandomFloat(5.0f, 10.0f);

			m_particles.push_back(particle);
		}
	}
}