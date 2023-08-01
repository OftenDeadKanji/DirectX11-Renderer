#pragma once
#include "../../math/mathUtils.h"

namespace Engine
{
	struct Transform
	{
		Transform() = default;
		Transform(const math::Vec3f& position, const math::Quat& rotation, const math::Vec3f& scale)
			: position(position), rotation(rotation), scale(scale)
		{}

		math::Vec3f position;
		math::Quat rotation;
		math::Vec3f scale;

		math::Mat4f transformMat;
		math::Mat4f transformInvMat;

		void updateMat()
		{
			math::Mat4f scaling;
			scaling <<
				scale.x(), 0.0f, 0.0f, 0.0f,
				0.0f, scale.y(), 0.0f, 0.0f,
				0.0f, 0.0f, scale.z(), 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
				;

			math::Mat4f rotating = math::Mat4f::Identity();
			math::QuatToMat3(rotation, rotating.block(0, 0, 3, 3));

			math::Mat4f translating = math::Mat4f::Identity();
			translating.row(3).head<3>() = position;

			transformMat = scaling * rotating * translating;
			transformInvMat = transformMat.inverse();
		}
	};
}