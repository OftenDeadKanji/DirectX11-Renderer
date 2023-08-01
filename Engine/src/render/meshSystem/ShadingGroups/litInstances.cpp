#include "LitInstances.h"
#include "../../camera/camera.h"
#include <algorithm>
#include "../../../utils/debug/debugOutput.h"
#include "../../../engine/renderer.h"
#include "../../../resourcesManagers/textureManager.h"
#include <locale>
#include <codecvt>
#include "../../lightSystem/lightSystem.h"

namespace Engine
{
	unsigned int LitInstances::getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const
	{
		return perModel[modelIndex].perMesh[meshIndex].perMaterial[materialIndex].instances[instanceIndex].objectID;
	}
	bool LitInstances::getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::LitInstance*& outObjectInstance)
	{
		for (auto& perModel : perModel)
		{
			for (auto& perMesh : perModel.perMesh)
			{
				for (auto& perMaterial : perMesh.perMaterial)
				{
					for (auto& instance : perMaterial.instances)
					{
						if (instance.objectID == objectID)
						{
							outObjectModel = &perModel;
							outObjectMesh = &perMesh;
							outObjectMaterial = &perMaterial;
							outObjectInstance = &instance;

							return true;
						}
					}
				}
			}
		}

		return false;
	}
	bool LitInstances::getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID)
	{
		for (auto& perModel : perModel)
		{
			for (auto& perMesh : perModel.perMesh)
			{
				for (auto& perMaterial : perMesh.perMaterial)
				{
					for (auto& instance : perMaterial.instances)
					{
						if (instance.objectID == objectID)
						{
							outID = instance.modelToWorldID;

							return true;
						}
					}
				}
			}
		}

		return false;
	}
	bool LitInstances::removeObjectByID(unsigned int objectID, PerModel& removedObject)
	{
		bool objectFound = false;

		for (auto& perModel : perModel)
		{
			for (auto& perMesh : perModel.perMesh)
			{
				for (auto& perMaterial : perMesh.perMaterial)
				{
					auto& instances = perMaterial.instances;
					for (int i = instances.size() - 1; i >= 0; i--)
					{
						if (instances[i].objectID == objectID)
						{
							if (removedObject.perMesh.empty())
							{
								removedObject.model = perModel.model;
							}
							PerMaterial removedPerMaterial;
							removedPerMaterial.material = perMaterial.material;
							removedPerMaterial.instances.push_back(instances[i]);

							PerMesh perMesh;
							perMesh.perMaterial.push_back(removedPerMaterial);

							removedObject.perMesh.push_back(perMesh);

							instances.erase(instances.begin() + i);

							objectFound = true;
						}
					}
				}
			}
		}

		return objectFound;
	}
	void LitInstances::updateInstanceBufferData(const PerMesh& perMesh, void* instanceBufferData, int& copiedNum, const Mesh& mesh, const Camera& camera)
	{
		auto* transformSystem = TransformSystem::getInstance();

		for (const auto& perMaterial : perMesh.perMaterial)
		{
			auto& instances = perMaterial.instances;
			int numModelInstances = instances.size();
			for (int index = 0; index < numModelInstances; index++)
			{
				InstanceInternal inst;
				
				inst.instanceNumber = instances[index].objectID;
				inst.modelToWorld = math::Mat4f::Identity();
				for (auto& m : mesh.instances)
				{
					inst.modelToWorld *= m;
				}
				inst.modelToWorld *= transformSystem->getMatrix(instances[index].modelToWorldID);

#if SHADER_CALCULATION_IN_CAMERA_CENTERED_WORLD_SPACE
				math::setTranslation(inst.modelToWorld, math::getTranslation(inst.modelToWorld) - camera.position());
#endif
				static_cast<InstanceInternal*>(instanceBufferData)[copiedNum++] = inst;
			}
		}
	}
	void LitInstances::initShader()
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc =
		{
			{"POS",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"COL",			0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16,								D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEX",			0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORM",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BTANG",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_VERTEX_DATA, 0},

			{"INS",			0, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, 0,								D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			1, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			2, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS",			3, DXGI_FORMAT_R32G32B32A32_FLOAT,	1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS_NUMBER",	0, DXGI_FORMAT_R32_UINT,			1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		shader.init(L"Shaders/lit/litVS.hlsl", L"Shaders/lit/litPS.hlsl", inputElementDesc);
		lineNormalVisualization.init(L"Shaders/lineNormalVisualization/lineNormalVisualizationVS.hlsl", L"", L"", L"Shaders/lineNormalVisualization/lineNormalVisualizationGS.hlsl", L"Shaders/lineNormalVisualization/lineNormalVisualizationPS.hlsl", inputElementDesc);

		stencilShader.init(L"Shaders/lit/litStencilVS.hlsl", L"", inputElementDesc);
		GBufferGeometryShader.init(L"Shaders/lit/litVS.hlsl", L"Shaders/lit/litGBufferPS.hlsl", inputElementDesc);
	}
	
	void LitInstances::bindMaterialData(const PerMaterial & perMaterial)
	{
		auto* devcon = D3D::getInstancePtr()->getDeviceContext();
		auto res = m_materialCBuffer.map(devcon);
		MaterialCBuffer* matPtr = static_cast<MaterialCBuffer*>(res.pData);

		if (perMaterial.material->textureDiffuse)
		{
			perMaterial.material->textureDiffuse->bindSRVForPS(20);
			matPtr->useDiffuseTexture = 1;
		}
		else
		{
			matPtr->useDiffuseTexture = 0;
		}

		if (perMaterial.material->textureNormal)
		{
			perMaterial.material->textureNormal->bindSRVForPS(21);
			matPtr->useNormalTexture = 1;
		}
		else
		{
			matPtr->useNormalTexture = 0;
		}

		if (perMaterial.material->textureARM)
		{
			perMaterial.material->textureARM->bindSRVForPS(22);

			if (perMaterial.material->useDefaultRoughness)
			{
				matPtr->useRoughnessTexture = 0;
				matPtr->roughness = ShadingGroupsDetails::LitMaterial::DEFAULT_ROUGHNESS;
			}
			else
			{
				matPtr->useRoughnessTexture = 1;
			}
			if (perMaterial.material->useDefaultMetalness)
			{
				matPtr->useMetalnessTexture = 0;
				matPtr->metalness = ShadingGroupsDetails::LitMaterial::DEFAULT_METALNESS;
			}
			else
			{
				matPtr->useMetalnessTexture = 1;
			}
		}
		else
		{
			matPtr->useRoughnessTexture = 0;
			matPtr->roughness = ShadingGroupsDetails::LitMaterial::DEFAULT_ROUGHNESS;

			matPtr->useMetalnessTexture = 0;
			matPtr->metalness = ShadingGroupsDetails::LitMaterial::DEFAULT_METALNESS;
		}

		m_materialCBuffer.unmap(devcon);

		m_materialCBuffer.setConstantBufferForPixelShader(devcon, 10);
	}
}