#pragma once
#include "../../math/box.h"
#include <limits>
#include <vector>
#include <memory>
#include <array>

namespace Engine
{
	struct Mesh;

	class TriangleOctree
	{
	public:
		TriangleOctree() = default;
		TriangleOctree(const TriangleOctree&) = delete;
		TriangleOctree& operator=(const TriangleOctree&) = delete;
		TriangleOctree(TriangleOctree&&) noexcept = default;
		TriangleOctree& operator=(TriangleOctree&&) noexcept = default;

		const static int PREFFERED_TRIANGLE_COUNT;
		const static float MAX_STRETCHING_RATIO;

		void clear()
		{
			m_mesh = nullptr;
		}

		bool inited() const
		{
			return m_mesh != nullptr;
		}

		void initialize(const Mesh& mesh);
		bool intersect(const math::Ray& ray, math::MeshIntersection& nearest) const;

	protected:
		const Mesh* m_mesh = nullptr;
		std::vector<uint32_t> m_triangles;

		math::Box m_boundingBox;
		math::Box m_initialBoundingBox;

		std::unique_ptr<std::array<TriangleOctree, 8>> m_children;

		void initialize(const Mesh& mesh, const math::Box& parentBoundingBox, const math::Vec3f& parentCenter, int octetIndex);

		bool addTriangle(uint32_t triangleIndex, const math::Vec3f& V1, const math::Vec3f& V2, const math::Vec3f& V3, const math::Vec3f& center);

		bool intersectInternal(const math::Ray& ray, math::MeshIntersection& outNearest) const;
	};
}