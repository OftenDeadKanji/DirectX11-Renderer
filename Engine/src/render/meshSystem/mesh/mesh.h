#pragma once
#include "../../math/triangle.h"
#include "../../math/box.h"
#include <vector>
#include "triangleOctree.h"

namespace Engine
{
	struct Mesh
	{
		Mesh() = default;
		Mesh(const std::vector<math::Triangle>& triangles)
			: triangles(triangles)
		{}

		std::string name;

		std::vector<math::Vertex> vertices;
		std::vector<math::Triangle> triangles;
		std::vector<math::Mat4f> instances;
		std::vector<math::Mat4f> instancesInv;

		math::Box boundingBox;
		TriangleOctree octree;

		void createBoundingBox();
		void initializeOctree();
	};
}