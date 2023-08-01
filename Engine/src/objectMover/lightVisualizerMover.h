#pragma once
#include "IObjectMover.h"
#include "../render/scene/scene.h"

namespace Engine
{
	class LightVisualizerMover
		: public IObjectMover
	{
	public:
		LightVisualizerMover(Scene::LightVisualizer* lightVisualizer)
			: m_lightVisualizer(lightVisualizer)
		{}

		virtual void move(const math::Vec3f& offset) override;

	private:
		Scene::LightVisualizer* m_lightVisualizer;
	};
}