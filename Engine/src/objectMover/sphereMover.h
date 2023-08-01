#pragma once
#include "IObjectMover.h"
#include "../render/scene/scene.h"

namespace Engine
{
	class SphereMover
		: public IObjectMover
	{
	public:
		SphereMover(Scene::Sphere* sphere)
			: m_sphere(sphere)
		{}

		virtual void move(const math::Vec3f& offset) override;

	private:
		Scene::Sphere* m_sphere;
	};
}