#pragma once
#include "IObjectMover.h"
#include "../render/scene/scene.h"

namespace Engine
{
	class MeshInstanceMover
		: public IObjectMover
	{
	public:
		MeshInstanceMover(Scene::MeshInstance* meshInstance)
			: m_meshInstance(meshInstance)
		{}

		virtual void move(const math::Vec3f& offset) override;

	private:
		Scene::MeshInstance* m_meshInstance;
	};
}
