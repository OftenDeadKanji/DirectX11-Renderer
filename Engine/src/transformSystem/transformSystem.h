#pragma once
#include "../utils/nonCopyable.h"
#include "../utils/containers/solidVector.h"
#include "../math/mathUtils.h"

namespace Engine
{
	class TransformSystem
		: public NonCopyable
	{
	private:
		TransformSystem() = default;

	public:
		using ID = SolidVector<math::Mat4f>::ID;

		static TransformSystem* createInstance();
		static void deleteInstance();
		static TransformSystem* getInstance();

		ID createMatrix();
		math::Mat4f& getMatrix(ID id);

		void clear();
	private:
		static TransformSystem* s_instance;

		SolidVector<math::Mat4f> m_transformMatrices;
	};
}
