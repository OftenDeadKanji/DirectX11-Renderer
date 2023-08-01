#include "mathUtils.h"

namespace Engine::math
{
	void invertOrthonormal(const math::Mat4f& src, math::Mat4f& dst)
	{
		dst.block(0, 0, 3, 3) = src.block(0, 0, 3, 3).transpose();

		const math::Vec3f& pos = src.row(3).head<3>();

		dst.row(3).head<3>() =
			-pos[0] * dst.row(0).head<3>()
			- pos[1] * dst.row(1).head<3>()
			- pos[2] * dst.row(2).head<3>();

		dst.col(3) = math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	}

	void invertOrthogonal(math::Mat4f& src, math::Mat4f& dst)
	{
		dst.block(0, 0, 3, 3) = src.block(0, 0, 3, 3).transpose();

		float lengthsXYZ[3] =
		{
			dst.row(0).head<3>().norm(),
			dst.row(1).head<3>().norm(),
			dst.row(2).head<3>().norm()
		};

		dst.row(0).head<3>() = math::Vec3f(1.0f, 1.0f, 1.0f).array() / (dst.row(0).head<3>() * lengthsXYZ[0]).array();
		dst.row(1).head<3>() = math::Vec3f(1.0f, 1.0f, 1.0f).array() / (dst.row(1).head<3>() * lengthsXYZ[1]).array();
		dst.row(2).head<3>() = math::Vec3f(1.0f, 1.0f, 1.0f).array() / (dst.row(2).head<3>() * lengthsXYZ[2]).array();

		const math::Vec3f& pos = src.row(3).head<3>();

		dst.row(3).head<3>() =
			-pos[0] * dst.row(0).head<3>() / lengthsXYZ[0]
			- pos[1] * dst.row(1).head<3>() / lengthsXYZ[1]
			- pos[2] * dst.row(2).head<3>() / lengthsXYZ[2];

		dst.col(3) = math::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	}
}