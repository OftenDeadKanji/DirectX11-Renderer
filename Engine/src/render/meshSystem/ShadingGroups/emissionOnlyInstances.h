#pragma once
#include "shadingGroup.h"

namespace Engine
{
	namespace ShadingGroupsDetails
	{
		struct EmissionOnlyInstance
		{
			TransformSystem::ID modelToWorldID;
			math::Vec3f color;
			unsigned int objectID;
		};
		struct EmissionOnlyMaterial
		{
			bool operator==(const EmissionOnlyMaterial& other)
			{
				return true;
			}
		};
	}

	class EmissionOnlyInstances
		: public ShadingGroup<ShadingGroupsDetails::EmissionOnlyMaterial, ShadingGroupsDetails::EmissionOnlyInstance>
	{
	public:
		EmissionOnlyInstances()
		{
			initShader();
		}
		unsigned int getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const override;
		bool getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::EmissionOnlyInstance*& outObjectInstance) override;
		bool getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID) override;
		bool removeObjectByID(unsigned int objectID, PerModel& removedObject) override;
	private:
		struct InstanceInternal
		{
			math::Mat4f modelToWorld;
			math::Vec3f color;
			unsigned int instanceNumber;
		};
		Buffer<InstanceInternal> m_instanceBuffer;

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
