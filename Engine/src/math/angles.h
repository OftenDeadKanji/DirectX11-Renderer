#pragma once
#include "mathUtils.h"

namespace Engine::math
{
	struct Angles
		: public Eigen::Matrix<float, 1, 3, Eigen::RowMajor>
	{
		Angles() = default;
		Angles(const math::Vec3f& v)
			: Eigen::Matrix<float, 1, 3, Eigen::RowMajor>(v)
		{
		}

		float& pitch()
		{
			return (*this)[0];
		}

		const float& pitch() const
		{
			return (*this)[0];
		}

		float& yaw()
		{
			return (*this)[1];
		}

		const float& yaw() const
		{
			return (*this)[1];
		}

		float& roll()
		{
			return (*this)[2];
		}

		const float& roll() const
		{
			return (*this)[2];
		}
	};
}