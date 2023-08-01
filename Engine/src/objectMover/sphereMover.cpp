#include "sphereMover.h"

namespace Engine
{
	void SphereMover::move(const math::Vec3f& offset)
	{
		m_sphere->position += offset;
	}
}