#pragma once
#include "shadingGroup.h"

namespace Engine
{
	namespace ShadingGroupsDetails
	{
		struct HologramInstance
		{
			TransformSystem::ID modelToWorldID;
			math::Vec3f color;
			unsigned int objectID;
		};
		struct HologramMaterial
		{
			bool operator==(const HologramMaterial& other)
			{
				return true;
			}
		};
	}

	class HologramInstances
		: public ShadingGroup<ShadingGroupsDetails::HologramMaterial, ShadingGroupsDetails::HologramInstance>
	{
	public:
		HologramInstances()
		{
			initShader();
		}
		unsigned int getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const override;
		bool getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::HologramInstance*& outObjectInstance) override;
		bool getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID) override;
		bool removeObjectByID(unsigned int objectID, PerModel& removedObject) override;

		void bindDepth2DShader() override;
		void bindDepthCubemapShader() override;
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

		Shader depth2DShader;
		Shader depthCubemapShader;
	};
}
