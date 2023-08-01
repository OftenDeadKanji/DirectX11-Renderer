#include "emissionOnlyInstances.h"
#include "../../camera/camera.h"
#include <algorithm>
#include "../../../utils/debug/debugOutput.h"
#include "../../../engine/renderer.h"

namespace Engine
{
	unsigned int EmissionOnlyInstances::getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const
	{
		return perModel[modelIndex].perMesh[meshIndex].perMaterial[materialIndex].instances[instanceIndex].objectID;
	}
	bool EmissionOnlyInstances::getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::EmissionOnlyInstance*& outObjectInstance)
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
	bool EmissionOnlyInstances::getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID)
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
	bool EmissionOnlyInstances::removeObjectByID(unsigned int objectID, PerModel& removedObject)
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
							PerMaterial perMaterial;
							perMaterial.material = perMaterial.material;
							perMaterial.instances.push_back(instances[i]);

							PerMesh perMesh;
							perMesh.perMaterial.push_back(perMaterial);

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
	void EmissionOnlyInstances::updateInstanceBufferData(const PerMesh& perMesh, void* instanceBufferData, int& copiedNum, const Mesh& mesh, const Camera& camera)
	{
		auto* transformSystem = TransformSystem::getInstance();

		for (const auto& perMaterial : perMesh.perMaterial)
		{
			auto& instances = perMaterial.instances;
			int numModelInstances = instances.size();
			for (int index = 0; index < numModelInstances; index++)
			{
				InstanceInternal inst;
				inst.color = instances[index].color;
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
	void EmissionOnlyInstances::initShader()
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
			{"INSCOL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS_NUMBER",	0, DXGI_FORMAT_R32_UINT,			1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		shader.init(L"Shaders/emissionOnly/emissionOnlyVS.hlsl", L"Shaders/emissionOnly/emissionOnlyPS.hlsl", inputElementDesc);
		lineNormalVisualization.init(L"Shaders/lineNormalVisualization/lineNormalVisualizationVS.hlsl", L"", L"", L"Shaders/lineNormalVisualization/lineNormalVisualizationGS.hlsl", L"Shaders/lineNormalVisualization/lineNormalVisualizationPS.hlsl", inputElementDesc);
		
		stencilShader.init(L"Shaders/emissionOnly/emissionOnlyStencilVS.hlsl", L"", inputElementDesc);

		//inputElementDesc.push_back({ "INS_NUMBER", 0, DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 });
		GBufferGeometryShader.init(L"Shaders/emissionOnly/emissionOnlyVS.hlsl", L"Shaders/emissionOnly/emissionOnlyGBufferPS.hlsl", inputElementDesc);
	}
	void EmissionOnlyInstances::bindMaterialData(const PerMaterial & perMaterial)
	{}
}