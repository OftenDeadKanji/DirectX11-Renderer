#pragma once
#include "../../math/mathUtils.h"
#include "../../math/ray.h"

namespace Engine
{
	class Camera
	{
	public:
		Camera& operator=(const Camera& other);

		void setPerspective(float fov, float aspect, float near, float far);
		void setOrthographic(float right, float left, float top, float bottom, float near, float far);

		void lookAt(const math::Vec3f& position, const math::Vec3f& target, const math::Vec3f& up = { 0.0f, 1.0f, 0.0f });

		math::Vec3f right() const
		{
			return m_viewInv.row(0).head<3>();
		}
		math::Vec3f top() const
		{
			return m_viewInv.row(1).head<3>();
		}
		math::Vec3f forward() const
		{
			return m_viewInv.row(2).head<3>();
		}
		math::Vec3f position() const
		{
			return m_viewInv.row(3).head<3>();
		}
		void position(const math::Vec3f& position)
		{
			m_viewInv.row(3).head<3>() = position;
		}

		void transformFromWorldToClipSpace(math::Vec4f& v) const;
		void transformFromClipToWorldSpace(math::Vec4f& v) const;

		void transformFromViewToClipSpace(math::Vec4f& v) const;
		void transformFromClipToViewSpace(math::Vec4f& v) const;

		void transformFromWorldToViewSpace(math::Vec4f& v) const;
		void transformFromViewtoWorldSpace(math::Vec4f& v) const;

		const math::Mat4f& getView() const
		{
			return m_view;
		}

		const math::Mat4f& getViewInv() const
		{
			return m_viewInv;
		}

		const math::Mat4f& getProj() const
		{
			return m_proj;
		}

		const math::Mat4f& getProjInv() const
		{
			return m_proj;
		}

		const math::Mat4f& getViewProj() const
		{
			return m_viewProj;
		}

		const math::Mat4f& getViewProjInv() const
		{
			return m_viewProjInv;
		}

		float getZNear() const;
		float getZFar() const;

		void setWorldOffset(const math::Vec3f& offset);
		void addWorldOffset(const math::Vec3f& offset);
		void addRelativeOffset(const math::Vec3f& offset);

		void setWorldAngles(const math::Angles& angles);
		void addWorldAngles(const math::Angles& angles);
		void addRelativeAngles(const math::Angles& angles);

		void updateBasis();
		void updateMatrices();
		void updateCamera();

		math::Ray generateRay(const math::Vec2i& pixelCoordinate, const math::Vec2i& resolution) const;

		void getCameraFrustumCornersDirections(math::Vec3f& toBL, math::Vec3f& fromBLtoTL, math::Vec3f& fromBLtoBR) const;

	private:
		math::Mat4f m_view = math::Mat4f::Identity();
		math::Mat4f m_proj = math::Mat4f::Identity();
		math::Mat4f m_viewProj = math::Mat4f::Identity();

		math::Mat4f m_viewInv = math::Mat4f::Identity();
		math::Mat4f m_projInv = math::Mat4f::Identity();
		math::Mat4f m_viewProjInv = math::Mat4f::Identity();

		Eigen::Quaternionf m_rotation = Eigen::Quaternionf::Identity();
		int m_rotationNormalizationIntervalCount = 0;
		const int m_rotationNormalizationFrequency = 20; // after how many updates should it be normalized

		float m_zNear, m_zFar;

		bool m_updatedBasis = false;
		bool m_updatedMatrices = false;

		math::Vec3f m_bottomLeftPositionWS, m_rightDirectionWS, m_upDirectionWS;
		math::Vec3f m_bottomLeftDirectionVS, m_rightDirectionVS, m_upDirectionVS;

		bool m_fixedBottom = true;
	};
}