#pragma once
#include "particle.h"
#include <vector>
#include "../../transformSystem/transformSystem.h"

namespace Engine
{
	class ParticleEmmiter
	{
	public:
		ParticleEmmiter(TransformSystem::ID transformID, float spawnRate, float spawnRadius, const math::Vec3f& particlesColor);

		void update(float deltaTime);

		const std::vector<Particle>& getParticles() const;
	private:
		void spawnParticles(int count);

		std::vector<Particle> m_particles;
		TransformSystem::ID m_transformID;
		float m_spawnRate;
		float m_spawnRadius;
		math::Vec3f m_particlesColor;

		float m_time;
		const float m_spawnTime = 0.5f;
		const float m_despawnTime = 1.5f;
		const float m_sizeIncreaseSpeed = 0.04f;
		const math::Vec2f m_initialSize = { 0.5f, 0.5f };
		float m_verticalSpeed;
	};
}