#pragma once
#include "IObjectMover.h"
#include "../math/mathUtils.h"
#include "../transformSystem/transformSystem.h"

namespace Engine
{
	class MatrixMover
		: public IObjectMover
	{
	public:
		MatrixMover(TransformSystem::ID matrixID)
			: m_matrix(matrixID)
		{}

		virtual void move(const math::Vec3f& offset) override;
	private:
		TransformSystem::ID m_matrix;
	};
}