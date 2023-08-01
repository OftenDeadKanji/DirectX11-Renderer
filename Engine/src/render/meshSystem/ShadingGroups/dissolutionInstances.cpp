#include "dissolutionInstances.h"
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
	unsigned int DissolutionInstances::getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const
	{
		return perModel[modelIndex].perMesh[meshIndex].perMaterial[materialIndex].instances[instanceIndex].objectID;
	}
	bool DissolutionInstances::getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, ShadingGroupsDetails::DissolutionInstance*& outObjectInstance)
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

	bool DissolutionInstances::getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID)
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

	bool DissolutionInstances::removeObjectByID(unsigned int objectID, PerModel& removedObject)
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

							PerMesh removedPerMesh;
							removedPerMesh.perMaterial.push_back(removedPerMaterial);

							removedObject.perMesh.push_back(removedPerMesh);

							instances.erase(instances.begin() + i);

							objectFound = true;
						}
					}
				}
			}
		}

		return objectFound;
	}

	void DissolutionInstances::bindDepth2DShader()
	{
		depth2DShader.bind();
	}
	void DissolutionInstances::bindDepthCubemapShader()
	{
		depthCubemapShader.bind();
	}
	void DissolutionInstances::updateInstanceBufferData(const PerMesh& perMesh, void* instanceBufferData, int& copiedNum, const Mesh& mesh, const Camera& camera)
	{
		auto* transformSystem = TransformSystem::getInstance();

		for (const auto& perMaterial : perMesh.perMaterial)
		{
			auto& instances = perMaterial.instances;
			int numModelInstances = instances.size();
			for (int index = 0; index < numModelInstances; index++)
			{
				InstanceInternal inst;
				inst.time = instances[index].time;
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
	void DissolutionInstances::initShader()
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
			{"INS_TIME",	0, DXGI_FORMAT_R32_FLOAT,			1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INS_NUMBER",	0, DXGI_FORMAT_R32_UINT,			1, D3D11_APPEND_ALIGNED_ELEMENT,	D3D11_INPUT_PER_INSTANCE_DATA, 1}
		};

		shader.init(L"Shaders/dissolution/dissolutionVS.hlsl", L"Shaders/dissolution/dissolutionPS.hlsl", inputElementDesc);
		lineNormalVisualization.init(L"Shaders/lineNormalVisualization/lineNormalVisualizationVS.hlsl", L"", L"", L"Shaders/lineNormalVisualization/lineNormalVisualizationGS.hlsl", L"Shaders/lineNormalVisualization/lineNormalVisualizationPS.hlsl", inputElementDesc);

		depth2DShader.init(L"Shaders/depth/depth2DdissolutionVS.hlsl", L"Shaders/depth/depth2DdissolutionPS.hlsl", inputElementDesc);
		depthCubemapShader.init(L"Shaders/depth/depthCubemapdissolutionVS.hlsl", L"", L"", L"Shaders/depth/depthCubemapdissolutionGS.hlsl", L"Shaders/depth/depthCubemapdissolutionPS.hlsl", inputElementDesc);

		stencilShader.init(L"Shaders/dissolution/dissolutionStencilVS.hlsl", L"Shaders/dissolution/dissolutionStencilPS.hlsl", inputElementDesc);
		GBufferGeometryShader.init(L"Shaders/dissolution/dissolutionVS.hlsl", L"Shaders/dissolution/dissolutionGBufferPS.hlsl", inputElementDesc);
	}
	
	void DissolutionInstances::bindMaterialData(const PerMaterial & perMaterial)
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
				matPtr->roughness = ShadingGroupsDetails::DissolutionMaterial::DEFAULT_ROUGHNESS;
			}
			else
			{
				matPtr->useRoughnessTexture = 1;
			}
			if (perMaterial.material->useDefaultMetalness)
			{
				matPtr->useMetalnessTexture = 0;
				matPtr->metalness = ShadingGroupsDetails::DissolutionMaterial::DEFAULT_METALNESS;
			}
			else
			{
				matPtr->useMetalnessTexture = 1;
			}
		}
		else
		{
			matPtr->useRoughnessTexture = 0;
			matPtr->roughness = ShadingGroupsDetails::DissolutionMaterial::DEFAULT_ROUGHNESS;

			matPtr->useMetalnessTexture = 0;
			matPtr->metalness = ShadingGroupsDetails::DissolutionMaterial::DEFAULT_METALNESS;
		}
		
		perMaterial.material->textureNoise->bindSRVForPS(23);

		m_materialCBuffer.unmap(devcon);

		m_materialCBuffer.setConstantBufferForPixelShader(devcon, 10);
	}
	void DissolutionInstances::createAndBindGBufferNormalCopy()
	{
		D3D11_TEXTURE2D_DESC texDesc{};
		Renderer::getInstancePtr()->getGBuffer().normal->GetDesc(&texDesc);

		DxResPtr<ID3D11Texture2D> gbufferNormalCopy;
		D3D::getInstancePtr()->getDevice()->CreateTexture2D(&texDesc, nullptr, gbufferNormalCopy.reset());

		D3D::getInstancePtr()->getDeviceContext()->CopyResource(gbufferNormalCopy, Renderer::getInstancePtr()->getGBuffer().normal);

		DxResPtr<ID3D11ShaderResourceView> srvNormal;
		D3D::getInstancePtr()->getDevice()->CreateShaderResourceView(gbufferNormalCopy, nullptr, srvNormal.reset());

		ID3D11ShaderResourceView* srvPtr[] = {
			srvNormal.ptr(),
		};

		D3D::getInstancePtr()->getDeviceContext()->PSSetShaderResources(24, 1, srvPtr);
	}
}