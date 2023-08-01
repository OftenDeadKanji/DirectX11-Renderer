#include "mesh.h"

namespace Engine
{
	void Mesh::createBoundingBox()
	{
		boundingBox.reset();

		for (auto& triangle : triangles)
		{
			boundingBox.expand(triangle.getVertex(0).position);
			boundingBox.expand(triangle.getVertex(1).position);
			boundingBox.expand(triangle.getVertex(2).position);
		}
	}

	void Mesh::initializeOctree()
	{
		octree.initialize(*this);
	}
}