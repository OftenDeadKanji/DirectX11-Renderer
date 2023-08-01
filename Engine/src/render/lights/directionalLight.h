#pragma once
#include "light.h"

namespace Engine
{
	class DirectionalLight : public Light
	{
	public:
		DirectionalLight() = default;
		DirectionalLight(const math::Vec3f& color, const math::Vec3f& direction)
			: Light(color), m_direction(direction.normalized())
		{}

		const math::Vec3f& getDirection() const
		{
			return m_direction;
		}
	private:
		math::Vec3f m_direction;
	};
}