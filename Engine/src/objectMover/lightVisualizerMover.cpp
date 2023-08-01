#include "lightVisualizerMover.h"

namespace Engine
{
	void LightVisualizerMover::move(const math::Vec3f& offset)
	{
		m_lightVisualizer->position += offset;

		switch (m_lightVisualizer->lightType)
		{
		case Scene::LightVisualizer::LightType::Point:
		{
			PointLight* pointLight = static_cast<PointLight*>(m_lightVisualizer->light);
			pointLight->setPos(pointLight->getPos() + offset);
		}
		break;
		case Scene::LightVisualizer::LightType::Spot:
		{
			SpotLight* spotLight = static_cast<SpotLight*>(m_lightVisualizer->light);
			spotLight->setPos(spotLight->getPos() + offset);
		}
		break;
		}
	}
}