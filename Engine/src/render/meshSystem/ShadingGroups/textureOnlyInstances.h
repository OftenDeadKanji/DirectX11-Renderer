#pragma once
#include "shadingGroup.h"

namespace Engine
{
	namespace ShadingGroupsDetails
	{
		struct TextureOnlyInstance
		{
			TransformSystem::ID modelToWorldID;
			unsigned int objectID;
		};
		struct TextureOnlyMaterial
		{
			std::shared_ptr<Texture> textureDiffuse;

			bool operator==(const TextureOnlyMaterial& other)
			{
				return textureDiffuse == other.textureDiffuse;
			}
		};
	}

	class TextureOnlyInstances
		: public ShadingGroup<ShadingGroupsDetails::TextureOnlyMaterial, ShadingGroupsDetails::TextureOnlyInstance>
	{
	public:
		TextureOnlyInstances()
		{
			initShader();
			m_materialCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
		}
		unsigned int getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const override;
		bool getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::TextureOnlyInstance*& outObjectInstance) override;
		bool getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID) override;
		bool removeObjectByID(unsigned int objectID, PerModel& removedObject) override;

	private:
		struct InstanceInternal
		{
			math::Mat4f modelToWorld;
			unsigned int instanceNumber;
		};
		Buffer<InstanceInternal> m_instanceBuffer;

		struct MaterialCBuffer
		{
			int32_t useDiffuseTexture;
			int32_t pad[3];
		};
		Buffer<MaterialCBuffer> m_materialCBuffer;

		virtual void createInstanceBuffer(int totalInstances) override
		{
			m_instanceBuffer.createInstanceBuffer(totalInstances, nullptr, D3D::getInstancePtr()->getDevice());
		}
		virtual void setInstanceBufferForIA(ID3D11DeviceContext4* devcon) override
		{
			m_instanceBuffer.setInstanceBufferForInputAssembler(devcon);
		}


		virtual void updateInstanceBufferData(const PerMesh& perMesh, void* instanceBufferData, int& copiedNum, const Mesh& mesh, const Camera& camera) override;
		virtual void* getInstanceBufferMappedData() override
		{
			auto& mappedResource = m_instanceBuffer.map(D3D::getInstancePtr()->getDeviceContext());
			return mappedResource.pData;
		}
		virtual void unmapInstanceBuffer() override
		{
			m_instanceBuffer.unmap(D3D::getInstancePtr()->getDeviceContext());
		}
		virtual void initShader() override;
		virtual void bindMaterialData(const PerMaterial& perMaterial) override;
	};
}
