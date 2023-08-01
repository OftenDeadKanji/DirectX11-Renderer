#pragma once
#include "mathUtils.h"
#include "vertex.h"

namespace Engine::math
{
	struct Ray;
	struct Intersection;
	struct MeshIntersection;

	struct Triangle
	{
		int vertexIndices[3];
		Vertex* verticesArray;

		Vec3f normal;

		Triangle() = default;
		Triangle(int v0, int v1, int v2, Vertex* verticesArray = nullptr)
			: vertexIndices{v0, v1, v2}, verticesArray(verticesArray)
		{
			if (verticesArray)
			{
				computeNormalVector();
			}
		}

		void computeNormalVector()
		{
			Vertex& v0 = *(verticesArray + vertexIndices[0]);
			Vertex& v1 = *(verticesArray + vertexIndices[1]);
			Vertex& v2 = *(verticesArray + vertexIndices[2]);

			normal = (v2.position - v0.position).cross(v1.position - v0.position).normalized();
		}

		Vertex& getVertex(int triangleVertexIndex)
		{
			assert(triangleVertexIndex >= 0 && triangleVertexIndex <= 3);

			return *(verticesArray + vertexIndices[triangleVertexIndex]);
		}

		bool isIntersecting(const Ray& ray, Intersection& outNearest) const;
		bool isIntersecting(const Ray& ray, MeshIntersection& outNearest) const;
	};
}