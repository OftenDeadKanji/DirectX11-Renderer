#include "planeMover.h"

namespace Engine
{
	void PlaneMover::move(const math::Vec3f& offset)
	{
		m_plane->point += offset;
	}
}