#pragma once
#include "../../math/mathUtils.h"
#include "../material/material.h"

namespace Engine
{
	class Light
	{
	public:
		Light() = default;
		Light(const math::Vec3f& color)
			: m_color(color)
		{}

		const math::Vec3f& getColor() const
		{
			return m_color;
		}

	protected:
		math::Vec3f m_color;
	};
}