#pragma once
#include "shadingGroup.h"

namespace Engine
{
	namespace ShadingGroupsDetails
	{
		struct LitInstance
		{
			TransformSystem::ID modelToWorldID;
			unsigned int objectID;
		};
		struct LitMaterial
		{
			std::string name;

			std::shared_ptr<Texture> textureDiffuse;
			std::shared_ptr<Texture> textureNormal;
			
			std::shared_ptr<Texture> textureARM; //AO-Roughness-Metallic
			bool useDefaultRoughness = false;
			bool useDefaultMetalness = false;

			static constexpr float DEFAULT_ROUGHNESS = 1.0f;
			static constexpr float DEFAULT_METALNESS = 0.0f;



			bool operator==(const LitMaterial& other)
			{
				return name == other.name;
			}
		};
	}

	class LitInstances
		: public ShadingGroup<ShadingGroupsDetails::LitMaterial, ShadingGroupsDetails::LitInstance>
	{
	public:
		LitInstances()
		{
			initShader();
			m_materialCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
		}
		unsigned int getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const override;
		bool getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::LitInstance*& outObjectInstance) override;
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
			int32_t useNormalTexture;
			int32_t useRoughnessTexture;
			float roughness;

			int32_t useMetalnessTexture;
			float metalness;
			int32_t pad[2];
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
