#include "triangleOctree.h"
#include "mesh.h"
#include "../../../math/ray.h"
#include "../../../math/intersection.h"
#include <algorithm>
#include "../../../utils/assert.h"

namespace Engine
{
	const int TriangleOctree::PREFFERED_TRIANGLE_COUNT = 32;
	const float TriangleOctree::MAX_STRETCHING_RATIO = 1.05f;

	inline const math::Vec3f& getPos(const Mesh& mesh, uint32_t triangleIndex, uint32_t vertexIndex)
	{
		int index = mesh.triangles.empty() ?
			triangleIndex * 3 + vertexIndex :
			mesh.triangles[triangleIndex].vertexIndices[vertexIndex];

		return mesh.vertices[index].position;

	}

	inline const math::Triangle& getTriangle(const Mesh& mesh, uint32_t triangleIndex)
	{
		return mesh.triangles[triangleIndex];
	}

	void TriangleOctree::initialize(const Mesh& mesh)
	{
		m_triangles.clear();
		m_triangles.shrink_to_fit();

		m_mesh = &mesh;
		m_children = nullptr;

		const math::Vec3f eps = { 1e-5f, 1e-5f, 1e-5f };
		m_boundingBox = m_initialBoundingBox = { mesh.boundingBox.min - eps, mesh.boundingBox.max + eps };

		for (uint32_t i = 0; i < mesh.triangles.size(); ++i)
		{
			const math::Vec3f& V1 = getPos(mesh, i, 0);
			const math::Vec3f& V2 = getPos(mesh, i, 1);
			const math::Vec3f& V3 = getPos(mesh, i, 2);

			math::Vec3f P = (V1 + V2 + V3) / 3.0f;

			bool inserted = addTriangle(i, V1, V2, V3, P);
			DEV_ASSERT(inserted);
		}
	}

	bool TriangleOctree::intersect(const math::Ray& ray, math::MeshIntersection& nearest) const
	{
		math::MeshIntersection tmp(nearest);
		if (!m_initialBoundingBox.intersects(ray, tmp))
		{
			return false;
		}

		return intersectInternal(ray, nearest);
	}

	void TriangleOctree::initialize(const Mesh& mesh, const math::Box& parentBoundingBox, const math::Vec3f& parentCenter, int octetIndex)
	{
		m_mesh = &mesh;
		m_children = nullptr;

		const float eps = 1e-5f;

		if (octetIndex % 2 == 0)
		{
			m_initialBoundingBox.min[0] = parentBoundingBox.min[0];
			m_initialBoundingBox.max[0] = parentCenter[0];
		}
		else
		{
			m_initialBoundingBox.min[0] = parentCenter[0];
			m_initialBoundingBox.max[0] = parentBoundingBox.max[0];
		}

		if (octetIndex % 4 < 2)
		{
			m_initialBoundingBox.min[1] = parentBoundingBox.min[1];
			m_initialBoundingBox.max[1] = parentCenter[1];
		}
		else
		{
			m_initialBoundingBox.min[1] = parentCenter[1];
			m_initialBoundingBox.max[1] = parentBoundingBox.max[1];
		}

		if (octetIndex < 4)
		{
			m_initialBoundingBox.min[2] = parentBoundingBox.min[2];
			m_initialBoundingBox.max[2] = parentCenter[2];
		}
		else
		{
			m_initialBoundingBox.min[2] = parentCenter[2];
			m_initialBoundingBox.max[2] = parentBoundingBox.max[2];
		}

		m_boundingBox = m_initialBoundingBox;
		math::Vec3f elongation = (MAX_STRETCHING_RATIO - 1.0f) * m_boundingBox.size();

		if (octetIndex % 2 == 0)
		{
			m_boundingBox.max[0] += elongation[0];
		}
		else
		{
			m_boundingBox.min[0] -= elongation[0];
		}

		if (octetIndex % 4 < 2)
		{
			m_boundingBox.max[1] += elongation[1];
		}
		else
		{
			m_boundingBox.min[1] -= elongation[1];
		}

		if (octetIndex < 4)
		{
			m_boundingBox.max[2] += elongation[2];
		}
		else
		{
			m_boundingBox.min[2] -= elongation[2];
		}
	}

	bool TriangleOctree::addTriangle(uint32_t triangleIndex, const math::Vec3f& V1, const math::Vec3f& V2, const math::Vec3f& V3, const math::Vec3f& center)
	{
		if (!m_initialBoundingBox.contains(center) ||
			!m_boundingBox.contains(V1) ||
			!m_boundingBox.contains(V2) ||
			!m_boundingBox.contains(V3)
			)
		{
			return false;
		}

		if (m_children == nullptr)
		{
			if (m_triangles.size() < PREFFERED_TRIANGLE_COUNT)
			{
				m_triangles.emplace_back(triangleIndex);
				return true;
			}
			else
			{
				math::Vec3f C = (m_initialBoundingBox.min + m_initialBoundingBox.max) / 2.0f;
				m_children.reset(new std::array<TriangleOctree, 8>());
				for (int i = 0; i < 8; ++i)
				{
					(*m_children)[i].initialize(*m_mesh, m_initialBoundingBox, C, i);
				}

				std::vector<uint32_t> newTriangles;

				for (uint32_t index : m_triangles)
				{
					const math::Vec3f& P1 = getPos(*m_mesh, index, 0);
					const math::Vec3f& P2 = getPos(*m_mesh, index, 1);
					const math::Vec3f& P3 = getPos(*m_mesh, index, 2);

					math::Vec3f P = (P1 + P2 + P3) / 3.0f;

					int i = 0;
					for (; i < 8; ++i)
					{
						if ((*m_children)[i].addTriangle(index, P1, P2, P3, P))
						{
							break;
						}
					}

					if (i == 8)
					{
						newTriangles.emplace_back(index);
					}
				}

				m_triangles = std::move(newTriangles);
			}
		}

		int i = 0;
		for (; i < 8; ++i)
		{
			if ((*m_children)[i].addTriangle(triangleIndex, V1, V2, V3, center))
			{
				break;
			}
		}

		if (i == 8)
		{
			m_triangles.emplace_back(triangleIndex);
		}

		return true;
	}

	bool TriangleOctree::intersectInternal(const math::Ray& ray, math::MeshIntersection& outNearest) const
	{
		{
			math::MeshIntersection tmp(outNearest);
			if (!m_boundingBox.intersects(ray, tmp))
			{
				return false;
			}
		}

		bool found = false;

		for (uint32_t i = 0; i < m_triangles.size(); ++i)
		{
			if (getTriangle(*m_mesh, m_triangles[i]).isIntersecting(ray, outNearest))
			{
				outNearest.triangle = i;
				found = true;
			}
		}

		if (!m_children)
		{
			return found;
		}

		struct OctantIntersection
		{
			int index;
			float t;
		};

		std::array<OctantIntersection, 8> boxIntersections;

		for (int i = 0; i < 8; ++i)
		{
			if ((*m_children)[i].m_boundingBox.contains(ray.origin))
			{
				boxIntersections[i].index = i;
				boxIntersections[i].t = 0.0f;
			}
			else
			{
				math::MeshIntersection tmp(outNearest);
				if ((*m_children)[i].m_boundingBox.intersects(ray, tmp))
				{
					boxIntersections[i].index = i;
					boxIntersections[i].t = tmp.t;
				}
				else
				{
					boxIntersections[i].index = -1;
				}
			}

		}
		std::sort(boxIntersections.begin(), boxIntersections.end(),
			[](const OctantIntersection& A, const OctantIntersection& B) -> bool
			{
				return A.t < B.t;
			});

		for (int i = 0; i < 8; ++i)
		{
			if (boxIntersections[i].index < 0 || boxIntersections[i].t > outNearest.t)
			{
				continue;
			}

			if ((*m_children)[boxIntersections[i].index].intersectInternal(ray, outNearest))
			{
				found = true;
			}
		}

		return found;
	}
}