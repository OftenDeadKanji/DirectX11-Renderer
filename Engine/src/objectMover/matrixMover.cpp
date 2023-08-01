#include "matrixMover.h"

namespace Engine
{
	void MatrixMover::move(const math::Vec3f& offset)
	{
		auto& mat = TransformSystem::getInstance()->getMatrix(m_matrix);
		mat.row(3).head<3>() += offset;
	}
}