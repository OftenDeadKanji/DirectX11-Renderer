#include "camera.h"
#include <cmath>
#include "../../math/mathUtils.h"

namespace Engine
{
	Camera& Camera::operator=(const Camera& other)
	{
		if (this != &other)
		{
			m_view = other.m_view;
			m_proj = other.m_proj;
			m_viewProj = other.m_viewProj;

			m_viewInv = other.m_viewInv;
			m_projInv = other.m_projInv;
			m_viewProjInv = other.m_viewProjInv;

			m_rotation = other.m_rotation;

			m_zNear = other.m_zNear;
			m_zFar = other.m_zFar;

			m_updatedBasis = other.m_updatedBasis;;
			m_updatedMatrices = other.m_updatedMatrices;;

			m_bottomLeftPositionWS = other.m_bottomLeftPositionWS, m_rightDirectionWS = other.m_rightDirectionWS, m_upDirectionWS = other.m_upDirectionWS;
			m_bottomLeftDirectionVS = other.m_bottomLeftDirectionVS, m_rightDirectionVS = other.m_rightDirectionVS, m_upDirectionVS = other.m_upDirectionVS;

			m_fixedBottom = other.m_fixedBottom;
		}

		return *this;
	}

	void Camera::setPerspective(float fov, float aspect, float near, float far)
	{
		m_proj = math::createPerspectiveProjectionMatrix(fov, aspect, near, far);
		m_projInv = m_proj.inverse();

		m_zNear = far;
		m_zFar = near;
	}

	void Camera::setOrthographic(float right, float left, float top, float bottom, float near, float far)
	{
		m_proj = math::createOrthographicProjectionMatrix(right, left, top, bottom, near, far);
		m_projInv = m_proj.inverse();

		m_zNear = near;
		m_zFar = far;
	}

	void Camera::lookAt(const math::Vec3f& position, const math::Vec3f& target, const math::Vec3f& up)
	{
		auto lookAtMatrix = math::lookAt(position, target, up);

		math::Mat3f quatMat = lookAtMatrix.block(0, 0, 3, 3).transpose();
		m_rotation = quatMat;

		setWorldOffset(position);

		m_updatedBasis = false;
	}

	void Camera::transformFromWorldToClipSpace(math::Vec4f& v) const
	{
		v *= m_viewProj;
		v /= v.w();
	}

	void Camera::transformFromClipToWorldSpace(math::Vec4f& v) const
	{
		v *= m_viewProjInv;
		v /= v.w();
	}

	void Camera::transformFromViewToClipSpace(math::Vec4f& v) const
	{
		v *= m_proj;
		v /= v.w();
	}

	void Camera::transformFromClipToViewSpace(math::Vec4f& v) const
	{
		v *= m_projInv;
		v /= v.w();
	}

	void Camera::transformFromWorldToViewSpace(math::Vec4f& v) const
	{
		v *= m_view;
	}

	void Camera::transformFromViewtoWorldSpace(math::Vec4f& v) const
	{
		v *= m_viewInv;
	}

	float Camera::getZNear() const
	{
		return m_zNear;
	}

	float Camera::getZFar() const
	{
		return m_zFar;
	}

	void Camera::setWorldOffset(const math::Vec3f& offset)
	{
		m_updatedMatrices = false;
		position(offset);
	}

	void Camera::addWorldOffset(const math::Vec3f& offset)
	{
		m_updatedMatrices = false;
		position(position() + offset);
	}

	void Camera::addRelativeOffset(const math::Vec3f& offset)
	{
		updateBasis();

		m_updatedMatrices = false;
		math::Vec3f worldOffset = offset[0] * right() + offset[1] * top() + offset[2] * forward();
		position(position() + worldOffset);
	}

	void Camera::setWorldAngles(const math::Angles& angles)
	{
		m_updatedBasis = false;
		m_updatedMatrices = false;

		m_rotation = Eigen::Quaternionf(Eigen::AngleAxisf(angles.roll(), math::Vec3f(0.0f, 0.0f, 1.0f)));
		m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.pitch(), math::Vec3f(1.0f, 0.0f, 0.0f)));
		m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.yaw(), math::Vec3f(0.0f, 1.0f, 0.0f )));

		m_rotation.normalize();
	}

	void Camera::addWorldAngles(const math::Angles& angles)
	{
		m_updatedBasis = false;
		m_updatedMatrices = false;

		m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.roll(), math::Vec3f(0.0f, 0.0f, 1.0f)));
		m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.pitch(), math::Vec3f(1.0f, 0.0f, 0.0f)));
		m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.yaw(), math::Vec3f(0.0f, 1.0f, 0.0f)));

		m_rotation.normalize();
	}

	void Camera::addRelativeAngles(const math::Angles& angles)
	{
		m_updatedBasis = false;
		m_updatedMatrices = false;

		auto forwardV = forward();
		auto rightV = right();
		auto topV = top();

		if(m_fixedBottom)
		{
			m_rotation *= math::Quat(Eigen::AngleAxisf(angles.pitch(), rightV));
			m_rotation *= math::Quat(Eigen::AngleAxisf(angles.yaw(), math::Vec3f(0.0f, 1.0f, 0.0f)));
		}
		else
		{
			m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.roll(), forwardV));
			m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.pitch(), rightV));
			m_rotation *= Eigen::Quaternionf(Eigen::AngleAxisf(angles.yaw(), topV));
		}
		m_rotation.normalize();
	}

	void Camera::updateBasis()
	{
		if (m_updatedBasis)
		{
			return;
		}
		m_updatedBasis = true;

		math::Mat3f rotMatrix = m_rotation.matrix();
		m_viewInv.block(0, 0, 3, 3) = rotMatrix;
	}

	void Camera::updateMatrices()
	{
		if (m_updatedMatrices)
		{
			return;
		}
		m_updatedMatrices = true;

		updateBasis();

		math::invertOrthonormal(m_viewInv, m_view);

		m_viewProj = m_view * m_proj;
		m_viewProjInv = m_projInv * m_viewInv;
	}

	void Camera::updateCamera()
	{
		updateBasis();
		updateMatrices();

		math::Vec4f BL(-1.f, -1.f, 1.0f, 1.0f);
		transformFromClipToViewSpace(BL);

		math::Vec4f BR(1.0f, -1.0f, 1.0f, 1.0f);
		transformFromClipToViewSpace(BR);

		math::Vec4f TL(-1.0f, 1.0f, 1.0f, 1.0f);
		transformFromClipToViewSpace(TL);

		math::Vec4f toTL = TL - BL;
		math::Vec4f toBR = BR - BL;

		m_bottomLeftDirectionVS = BL.head<3>();
		m_rightDirectionVS = toBR.head<3>();
		m_upDirectionVS = toTL.head<3>();

		transformFromViewtoWorldSpace(BL);
		transformFromViewtoWorldSpace(toBR);
		transformFromViewtoWorldSpace(toTL);

		m_bottomLeftPositionWS = BL.head<3>();
		m_rightDirectionWS = toBR.head<3>();
		m_upDirectionWS = toTL.head<3>();
	}

	math::Ray Camera::generateRay(const math::Vec2i& pixelCoordinate, const math::Vec2i& resolution) const
	{
		math::Vec2f position{
			(pixelCoordinate.x() + 0.5f) / resolution.x(),
			(pixelCoordinate.y() + 0.5f) / resolution.y()
		};

		math::Vec3f pixelPosition = m_bottomLeftPositionWS + m_rightDirectionWS * position.x() + m_upDirectionWS * position.y();

		math::Vec3f direction = (pixelPosition - this->position()).normalized();

		return { this->position(), direction };
	}
	void Camera::getCameraFrustumCornersDirections(math::Vec3f& toBL, math::Vec3f& fromBLtoTL, math::Vec3f& fromBLtoBR) const
	{
		math::Vec4f BL(m_bottomLeftDirectionVS.x(), m_bottomLeftDirectionVS.y(), m_bottomLeftDirectionVS.z(), 0.0f);
		transformFromViewtoWorldSpace(BL);
		
		math::Vec4f BR(m_rightDirectionVS.x(), m_rightDirectionVS.y(), m_rightDirectionVS.z(), 0.0f);
		transformFromViewtoWorldSpace(BR);

		math::Vec4f TL(m_upDirectionVS.x(), m_upDirectionVS.y(), m_upDirectionVS.z(), 0.0f);
		transformFromViewtoWorldSpace(TL);

		toBL = BL.head<3>();;
		fromBLtoTL = TL.head<3>();
		fromBLtoBR = BR.head<3>();
	}
}
