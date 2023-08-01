#include "meshInstanceMover.h"

namespace Engine
{
	void MeshInstanceMover::move(const math::Vec3f& offset)
	{
		m_meshInstance->transform.position += offset;
		m_meshInstance->transform.updateMat();
	}
}