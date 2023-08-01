#pragma once
#define _USE_MATH_DEFINES
#include<cmath>
#include "../dependencies/Eigen/Eigen/Dense"

namespace Engine::math
{
	using Vec2ui8 = Eigen::Matrix<uint8_t, 1, 2, Eigen::RowMajor>;
	using Vec3ui8 = Eigen::Matrix<uint8_t, 1, 3, Eigen::RowMajor>;
	using Vec4ui8 = Eigen::Matrix<uint8_t, 1, 4, Eigen::RowMajor>;

	using Vec2i = Eigen::Matrix<int, 1, 2, Eigen::RowMajor>;
	using Vec3i = Eigen::Matrix<int, 1, 3, Eigen::RowMajor>;
	using Vec4i = Eigen::Matrix<int, 1, 4, Eigen::RowMajor>;

	using Vec2f = Eigen::Matrix<float, 1, 2, Eigen::RowMajor>;
	using Vec3f = Eigen::Matrix<float, 1, 3, Eigen::RowMajor>;
	using Vec4f = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>;

	using Mat3f = Eigen::Matrix<float, 3, 3, Eigen::RowMajor>;
	using Mat4f = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

	using Quat = Eigen::Quaternionf;

	struct Angles
		: public Vec3f
	{
		Angles() = default;
		Angles(const Vec3f& v)
			: Vec3f(v)
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

	constexpr float PI = 3.141592653589793238f;

	void invertOrthonormal(const math::Mat4f& src, math::Mat4f& dst);
	void invertOrthogonal(math::Mat4f& src, math::Mat4f& dst);

	inline float deg2rad(float deg)
	{
		return (deg * PI) / 180.0f;
	}

	inline void QuatToMat3(const math::Quat q, math::Mat3f outMat)
	{
		outMat <<
			1.0f - 2.0f * q.y() * q.y() - 2.0f * q.z() * q.z(),		2.0f * q.x() * q.y() - 2.0f * q.z() * q.w(),			2.0f * q.x() * q.z() + 2.0f * q.y() * q.w(),
			2.0f * q.x() * q.y() + 2.0f * q.z() * q.w(),			1.0f - 2.0f * q.x() * q.x() - 2.0f * q.z() * q.z(),		2.0f * q.y() * q.z() - 2.0f * q.x() * q.w(),
			2.0f * q.x() * q.z() - 2.0f * q.y() * q.w(),			2.0f * q.y() * q.z() + 2.0f * q.x() * q.w(),			1.0f - 2.0f * q.x() * q.x() - 2.0f * q.y() * q.y();

	}

	inline math::Mat4f createPerspectiveProjectionMatrix(float fov, float aspect, float _near, float _far)
	{
		auto ctg = 1.0f / (std::tanf(math::deg2rad(fov * 0.5f)));
		
		math::Mat4f projMat;
		projMat << (ctg / aspect), 0, 0, 0,
			0, (ctg), 0, 0,
			0, 0, (_near / (_near - _far)), 1,
			0, 0, (-_far * _near / (_near - _far)), 0;

		return projMat;
	}

	inline math::Mat4f createOrthographicProjectionMatrix(float right, float left, float top, float bottom, float near, float far)
	{
		math::Mat4f orthoMat;
		orthoMat <<
			2.0f / (right - left), 0, 0, 0,
			0, 2.0f / (top - bottom), 0, 0,
			0, 0, 1.0f / (near - far), 0,
			(left + right) / (left - right), (top + bottom) / (bottom - top), -far / (near - far), 1.0f;

		return orthoMat;
	}

	inline Mat4f lookAt(const Vec3f& position, const Vec3f& target, const Vec3f& up = { 0.0f, 1.0f, 0.0f })
	{
		Vec3f f = (target - position).normalized();
		Vec3f r = up.cross(f).normalized();
		Vec3f u = f.cross(r);

		//Mat4f view;
		//view <<
		//	r.x(), r.y(), r.z(), 0.0f,
		//	u.x(), u.y(), u.z(), 0.0f,
		//	f.x(), f.y(), f.z(), 0.0f,
		//	position.x(), position.y(), position.z(), 1.0f;
		//
		//Mat4f viewInv;
		//invertOrthonormal(view, viewInv);

		Mat4f viewMatrix;
		viewMatrix <<
			r.x(), u.x(), f.x(), 0.0f,
			r.y(), u.y(), f.y(), 0.0f,
			r.z(), u.z(), f.z(), 0.0f,
			-r.dot(position), -u.dot(position), -f.dot(position), 1.0f;

		return viewMatrix;
	}

	inline void setTranslation(Mat4f& mat, const Vec3f& translation)
	{
		mat.row(3).head<3>() = translation;
	}
	
	inline math::Vec3f getTranslation(const Mat4f& mat)
	{
		return mat.row(3).head<3>();
	}

	inline math::Vec3f getForward(const Mat4f& mat)
	{
		return mat.row(2).head<3>();
	}

	inline void setScale(Mat4f& mat, const Vec3f& scale)
	{
		mat(0, 0) = scale.x();
		mat(1, 1) = scale.y();
		mat(2, 2) = scale.z();
	}

	

}