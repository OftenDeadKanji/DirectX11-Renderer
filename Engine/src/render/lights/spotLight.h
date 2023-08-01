#pragma once
#include "light.h"

namespace Engine
{
	class SpotLight
		: public Light
	{
	public:
		SpotLight() = default;
		SpotLight(const math::Vec3f& color, const math::Vec3f& position, const math::Vec3f& direction, float angle)
			: Light(color), m_position(position), m_direction(direction), m_angle(std::cosf(math::deg2rad(angle)))
		{}

		bool isInside(const math::Vec3f& point) const
		{
			float angleToPoint = (point - m_position).normalized().dot(m_direction);

			return angleToPoint > m_angle;
		}

		const math::Vec3f& getPos() const
		{
			return m_position;
		}

		void setPos(const math::Vec3f& position)
		{
			m_position = position;
		}


		const math::Vec3f& getDirection() const
		{
			return m_direction;
		}
	private:
		math::Vec3f m_position;
		math::Vec3f m_direction;
		float m_angle;
	};
}
