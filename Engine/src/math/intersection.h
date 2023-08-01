#pragma once
#include "mathUtils.h"

namespace Engine::math
{
	struct Intersection
	{
		float t;
		math::Vec3f position;
		math::Vec3f normal;

		bool valid() const
		{
			return std::isfinite(t);
		}

		void reset()
		{
			t = std::numeric_limits<float>::infinity();
		}
	};

	struct MeshIntersection
	{
		math::Vec3f position;
		math::Vec3f normal;
		float _near; //for some reason, Visual gives error when it's named 'near' (apparently 'far' and 'near' are key words)
		float t;
		uint32_t triangle;
	
		void reset(float _near, float _far = std::numeric_limits<float>::infinity())
		{
			this->_near = _near;
			t = _far;
		}
	
		bool valid() const
		{
			return std::isfinite(t);
		}
	};
}