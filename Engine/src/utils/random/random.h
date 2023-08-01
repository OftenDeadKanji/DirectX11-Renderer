#pragma once
#include "../nonCopyable.h"
#include <random>
#include "../../math/mathUtils.h"

namespace Engine
{
	class Random
		: public NonCopyable
	{
	public:
		static Random* createInstance();
		static Random* getInstance();
		static void deleteInstance();

		void init();

		float getRandomFloat(float min = 0.0f, float max = 1.0f);
		math::Vec2f getRandomVec2f(float minR = 1.0f, float maxR = 1.0f);
		math::Vec3f getRandomVec3f(float minR = 1.0f, float maxR = 1.0f);

		float noise(float x, float y, float z);
	private:
		static Random* s_instance;

		std::uniform_real_distribution<float> m_distribution;
		std::default_random_engine m_randomEngine;

		//perlin noise https://mrl.cs.nyu.edu/~perlin/noise/
		std::array<int, 512> p;
		std::array<int, 256> permutations;
	};
}