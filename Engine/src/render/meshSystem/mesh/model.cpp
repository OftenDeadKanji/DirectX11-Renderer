#include "model.h"
#include "../../../utils/assert.h"

namespace Engine
{
	void Model::createVertexBuffer()
	{
		if (m_meshes.empty())
		{
			return;
		}

		std::vector<math::Vertex> vertices;
		std::vector<unsigned int> indices;
		for (const auto& mesh : m_meshes)
		{
			int offset = vertices.size();
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			
			int num = vertices.size() - offset;
			
			int indOffset = indices.size();
			for (const auto& triangle : mesh.triangles)
			{
				indices.push_back(triangle.vertexIndices[0]);
				indices.push_back(triangle.vertexIndices[1]);
				indices.push_back(triangle.vertexIndices[2]);
			}
			int indNum = indices.size() - indOffset;

			MeshRange range = { offset, indOffset, num, indNum };
			m_ranges.push_back(MeshRange(range));
		}
		
		auto device = D3D::getInstancePtr()->getDevice();
		m_vertices.createVertexBuffer(vertices.size(), vertices.data(), device);
		if (!indices.empty())
		{
			m_indices.createIndexBuffer(indices.size(), indices.data(), device);
		}
	}
	void Model::setVertexBufferForIA()
	{
		m_vertices.setVertexBufferForInputAssembler(D3D::getInstancePtr()->getDeviceContext());
	}
	void Model::setIndexBufferForIA()
	{
		m_indices.setIndexBufferForInputAssembler(D3D::getInstancePtr()->getDeviceContext());
	}
	const Mesh& Model::getMesh(int index) const
	{
		return m_meshes[index];
	}
	const Model::MeshRange& Model::getMeshRange(int index) const
	{
		return m_ranges[index];
	}
	const math::Box& Model::getBoundingBox() const
	{
		return boundingBox;
	}
	const std::vector<Mesh>& Model::getMeshes() const
	{
		return m_meshes;
	}
}