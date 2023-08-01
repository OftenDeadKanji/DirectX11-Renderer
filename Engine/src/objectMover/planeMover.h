#pragma once
#include "IObjectMover.h"
#include "../render/scene/scene.h"

namespace Engine
{
	class PlaneMover
		: public IObjectMover
	{
	public:
		PlaneMover(Scene::Plane* plane)
			: m_plane(plane)
		{}

		virtual void move(const math::Vec3f& offset) override;

	private:
		Scene::Plane* m_plane;
	};
}