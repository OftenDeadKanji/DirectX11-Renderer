#pragma once
#include "../math/mathUtils.h"

namespace Engine
{
	class IObjectMover
	{
	public:
		virtual void move(const math::Vec3f& offset) = 0;
	};
}