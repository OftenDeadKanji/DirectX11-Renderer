#pragma once
#include "light.h"

namespace Engine
{
	class PointLight
		: public Light
	{
	public:
		PointLight() = default;
		PointLight(const math::Vec3f& color, const math::Vec3f& position)
			: Light(color), m_position(position)
		{}

		const math::Vec3f& getPos() const
		{
			return m_position;
		}

		void setPos(const math::Vec3f& position)
		{
			m_position = position;
		}
	private:
		math::Vec3f m_position;
	};
}
