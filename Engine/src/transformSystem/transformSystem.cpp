#include "transformSystem.h"

namespace Engine
{
	TransformSystem* TransformSystem::s_instance = nullptr;

	TransformSystem* TransformSystem::createInstance()
	{
		return s_instance = new TransformSystem();
	}
	void TransformSystem::deleteInstance()
	{
		s_instance->clear();
		delete s_instance;
		s_instance = nullptr;
	}
	TransformSystem* TransformSystem::getInstance()
	{
		return s_instance;
	}
	TransformSystem::ID TransformSystem::createMatrix()
	{
		return m_transformMatrices.insert(math::Mat4f::Identity());
	}

	math::Mat4f& TransformSystem::getMatrix(ID id)
	{
		return m_transformMatrices.at(id);
	}
	void TransformSystem::clear()
	{
		m_transformMatrices.clear();
	}
}