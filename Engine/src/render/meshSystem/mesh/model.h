#pragma once
#include "mesh.h"
#include <string>
#include "../../Direct3d/buffer.h"

namespace Engine
{
	class ModelManager;
	template<typename T, typename K>
	class ShadingGroup;

	class Model
	{
		friend ModelManager;
		template<typename T, typename K>
		friend class ShadingGroup;

	public:
		struct MeshRange
		{
			int vertexOffset;
			int indexOffset;
			int vertexNum;
			int indexNum;
		};
		
		void createVertexBuffer();
		void setVertexBufferForIA();
		void setIndexBufferForIA();

		const std::vector<Mesh>& getMeshes() const;
		const Mesh& getMesh(int index) const;
		const MeshRange& getMeshRange(int index) const;

		const math::Box& getBoundingBox() const;
	private:
		std::vector<Mesh> m_meshes;
		std::vector<MeshRange> m_ranges;
		Buffer<math::Vertex> m_vertices;
		Buffer<unsigned int> m_indices;

		std::string name;
		math::Box boundingBox;
	};
}
