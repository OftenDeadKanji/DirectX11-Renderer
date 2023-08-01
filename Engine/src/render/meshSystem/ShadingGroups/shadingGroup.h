#pragma once
#include "../../../math/mathUtils.h"
#include <vector>
#include "../mesh/model.h"
#include "../../../utils/assert.h"
#include "../../shader/shader.h"
#include <chrono>
#include "../../../transformSystem/transformSystem.h"
#include "../../../engine/renderer.h"
#include "../../../utils/debug/debugOutput.h"
#include "../../camera/camera.h"
#include <algorithm>
#include "../../../math/intersection.h"
#include "../../../math/ray.h"
#include "../../lightSystem/lightSystem.h"

namespace Engine
{
	template <typename Material, typename Instance>
	class ShadingGroup
	{
	public:
		struct PerMaterial
		{
			std::shared_ptr<Material> material;
			std::vector<Instance> instances;
		};

		struct PerMesh
		{
			std::vector<PerMaterial> perMaterial;
		};

		struct PerModel
		{
			std::shared_ptr<Model> model;
			std::vector<PerMesh> perMesh;
		};

	public:
		ShadingGroup()
		{
			std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc =
			{
				{"POS",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COL",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TEX",		0, DXGI_FORMAT_R32G32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"NORM",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"TANG",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"BTANG",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

				{"INS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
				{"INS", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
				{"INS", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
				{"INS", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}
			};

			depth2DShader.init(L"Shaders/depth/depth2DVS.hlsl", L"", inputElementDesc);
			depthCubemapShader.init(L"Shaders/depth/depthCubemapVS.hlsl", L"", L"", L"Shaders/depth/depthCubemapGS.hlsl", L"", inputElementDesc);

			depthCubemapCBuffer.createConstantBuffer(D3D::getInstancePtr()->getDevice());
		}

		virtual PerModel add(std::shared_ptr<Model> model, std::shared_ptr<Material> material, Instance instance)
		{
			PerModel addedInstances;
			addedInstances.model = model;

			auto compare = [model](const PerModel& right) 
			{ 
				return model == right.model;
			};

			auto modelIter = std::find_if(perModel.begin(), perModel.end(), compare);
			if (modelIter == perModel.end())
			{
				std::vector<PerMesh> emptyPerMesh(model->m_meshes.size());
				PerModel perModel = { model, emptyPerMesh };

				this->perModel.push_back(perModel);
				modelIter = std::prev(this->perModel.end());
			}

			bool added = false;
			for (auto& perMesh : modelIter->perMesh)
			{
				addedInstances.perMesh.push_back({});
				for (auto& perMaterial : perMesh.perMaterial)
				{
					if ((*perMaterial.material).operator==(*material))
					{
						perMaterial.instances.push_back(instance);
						added = true;

						addedInstances.perMesh.back().perMaterial.push_back({material, {instance}});
					}
				}
			}

			if (added)
			{
				return addedInstances;
			}

			for (int i = 0; i < model->m_meshes.size(); i++)
			{
				auto newMaterialForMesh = std::make_shared<Material>();
				*newMaterialForMesh = *material;

				PerMaterial perMaterial = { newMaterialForMesh, {instance} };
				modelIter->perMesh[i].perMaterial.push_back(perMaterial);

				addedInstances.perMesh[i].perMaterial.push_back({ newMaterialForMesh, {instance}});
			}

			return addedInstances;
		}
		
		void add(PerModel& objectToAdd)
		{
			bool added = false;
			for (auto& perModel : perModel)
			{
				if (perModel.model == objectToAdd.model)
				{
					added = true;

					for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
					{
						auto& perMesh = perModel.perMesh[meshIndex];
						auto& objectToAddPerMesh = objectToAdd.perMesh[meshIndex];

						bool addedMeshInstances = false;
						for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
						{
							auto& perMaterial = perMesh.perMaterial[materialIndex];
							auto& objectToAddPerMaterial = objectToAddPerMesh.perMaterial.front();
							
							if ((*perMaterial.material).operator==(*objectToAddPerMaterial.material))
							{
								for (int instanceIndex = 0; instanceIndex < objectToAddPerMaterial.instances.size(); instanceIndex++)
								{
									perMaterial.instances.push_back(objectToAddPerMaterial.instances[instanceIndex]);
									addedMeshInstances = true;
								}
							}
						}

						if (!addedMeshInstances)
						{
							for (auto& perMaterialToAdd : objectToAddPerMesh.perMaterial)
							{
								perMesh.perMaterial.push_back(perMaterialToAdd);
							}
						}
					}
				}
			}

			if (!added)
			{
				perModel.push_back(objectToAdd);
			}
		}
		void updateInstanceBuffers(const Camera& camera)
		{
			int totalInstances = 0;
			for (auto& perModel : perModel)
			{
				for (auto& perMesh : perModel.perMesh)
				{
					for (const auto& perMaterial : perMesh.perMaterial)
					{
						totalInstances += int(perMaterial.instances.size());
					}
				}
			}

			if (totalInstances == 0)
			{
				return;
			}

			auto* device = D3D::getInstancePtr()->getDevice();
			createInstanceBuffer(totalInstances);

			auto devcon = D3D::getInstancePtr()->getDeviceContext();
			void* instanceBufferData = getInstanceBufferMappedData();
			
			int copiedNum = 0;
			for (const auto& perModel : perModel)
			{
				for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
				{
					const Mesh& mesh = perModel.model->m_meshes[meshIndex];
					updateInstanceBufferData(perModel.perMesh[meshIndex], instanceBufferData, copiedNum, mesh, camera);
				}
			}
			
			unmapInstanceBuffer();
		}
		void render()
		{
			render(shader);
			if (m_isNormalVisualizationOn)
			{
				render(lineNormalVisualization);
			}
		}

		virtual void bindDepth2DShader()
		{
			depth2DShader.bind();
		}
		virtual void bindDepthCubemapShader()
		{
			depthCubemapShader.bind();
		}
		void renderDepth2D()
		{
			auto* devcon = D3D::getInstancePtr()->getDeviceContext();

			setInstanceBufferForIA(devcon);
			int renderedInstances = 0;
			for (const auto& perModel : perModel)
			{
				if (!perModel.model)
				{
					_DEBUG_OUTPUT("Model is nullptr!")
						continue;
				}

				perModel.model->m_vertices.setVertexBufferForInputAssembler(devcon);
				perModel.model->m_indices.setIndexBufferForInputAssembler(devcon);

				for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
				{
					const Mesh& mesh = perModel.model->m_meshes[meshIndex];
					const auto& meshRange = perModel.model->m_ranges[meshIndex];

					for (const auto& perMaterial : perModel.perMesh[meshIndex].perMaterial)
					{
						if (perMaterial.instances.empty())
						{
							continue;
						}

						bindMaterialData(perMaterial);

						unsigned int numInstances = static_cast<unsigned int>(perMaterial.instances.size());
						if (perModel.model->m_indices.isEmpty())
						{
							D3D::getInstancePtr()->getDeviceContext()->DrawInstanced(meshRange.vertexNum, numInstances, meshRange.vertexOffset, renderedInstances);
						}
						else
						{
							D3D::getInstancePtr()->getDeviceContext()->DrawIndexedInstanced(meshRange.indexNum, numInstances, meshRange.indexOffset, meshRange.vertexOffset, renderedInstances);
						}
						renderedInstances += numInstances;
					}
				}
			}
		}

		void renderDepthCubemaps(const std::vector<math::Vec3f>& positions)
		{
			auto* devcon = D3D::getInstancePtr()->getDeviceContext();

			setInstanceBufferForIA(devcon);
			int renderedInstances = 0;
			for (const auto& perModel : perModel)
			{
				if (!perModel.model)
				{
					_DEBUG_OUTPUT("Model is nullptr!")
						continue;
				}

				perModel.model->m_vertices.setVertexBufferForInputAssembler(devcon);
				perModel.model->m_indices.setIndexBufferForInputAssembler(devcon);

				for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
				{
					const Mesh& mesh = perModel.model->m_meshes[meshIndex];
					const auto& meshRange = perModel.model->m_ranges[meshIndex];

					for (const auto& perMaterial : perModel.perMesh[meshIndex].perMaterial)
					{
						if (perMaterial.instances.empty())
						{
							continue;
						}
						bindMaterialData(perMaterial);
						unsigned int numInstances = static_cast<unsigned int>(perMaterial.instances.size());
						if (perModel.model->m_indices.isEmpty())
						{
							D3D::getInstancePtr()->getDeviceContext()->DrawInstanced(meshRange.vertexNum, numInstances, meshRange.vertexOffset, renderedInstances);
						}
						else
						{
							for (int i = 0; i < positions.size(); i++)
							{

								auto mappedRes = depthCubemapCBuffer.map(devcon);
								PerDepthCubemapData* ptr = static_cast<PerDepthCubemapData*>(mappedRes.pData);
								ptr->index = i;
								depthCubemapCBuffer.unmap(devcon);
								
								depthCubemapCBuffer.setConstantBufferForVertexShader(devcon, 10);
								depthCubemapCBuffer.setConstantBufferForGeometryShader(devcon, 10);
								depthCubemapCBuffer.setConstantBufferForPixelShader(devcon, 10);

								LightSystem::getInstancePtr()->setPerFrameBufferForVS(devcon);
								LightSystem::getInstancePtr()->setPerFrameBufferForGS(devcon);
								LightSystem::getInstancePtr()->setPerFrameBufferForPS(devcon);

								D3D::getInstancePtr()->getDeviceContext()->DrawIndexedInstanced(meshRange.indexNum, numInstances, meshRange.indexOffset, meshRange.vertexOffset, renderedInstances);
							}
						}
						renderedInstances += numInstances;
					}
				}
			}
		}

		void renderStencil()
		{
			render(stencilShader);
		}

		void renderGBufferGeometry()
		{
			render(GBufferGeometryShader);
		}

		std::vector<PerModel>& getModels()
		{
			return perModel;
		}
		void clear()
		{
			perModel.clear();
			//instanceBuffer.reset();
			//emissionBuffer.reset();
			shader.reset();
		}
		void setNormalVisualization(bool state)
		{
			m_isNormalVisualizationOn = state;
		}

		void findIntersection(const math::Ray& ray, math::Intersection& outIntersection, TransformSystem::ID& outMatrixID, unsigned int& objectID)
		{
			auto* transformSystem = TransformSystem::getInstance();
			math::Ray rayInModelSpace;

			for (int modelIndex = 0; modelIndex < perModel.size(); modelIndex++)
			{
				auto& perModel = this->perModel[modelIndex];

				if (!perModel.model)
				{
					continue;
				}

				for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
				{
					auto& perMesh = perModel.perMesh[meshIndex];
					for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
					{
						auto& perMaterial = perMesh.perMaterial[materialIndex];
						for (int instanceIndex = 0; instanceIndex < perMaterial.instances.size(); instanceIndex++)
						{
							auto& instance = perMaterial.instances[instanceIndex];

							auto& mesh = perModel.model->getMeshes()[meshIndex];

							math::Mat4f modelToWorld = transformSystem->getMatrix(instance.modelToWorldID);
							math::Mat4f meshToModel = mesh.instances[0];

							math::Mat4f transform = meshToModel * modelToWorld;
							math::Mat4f transformInv = transform.inverse();

							rayInModelSpace.origin = (math::Vec4f(ray.origin.x(), ray.origin.y(), ray.origin.z(), 1.0f) * transformInv).head<3>();
							rayInModelSpace.direction = (math::Vec4f(ray.direction.x(), ray.direction.y(), ray.direction.z(), 0.0f) * transformInv).head<3>();

							math::MeshIntersection intersection;
							intersection.reset(0.0f);
							intersection.t = outIntersection.t;

							if (mesh.octree.intersect(rayInModelSpace, intersection))
							{
								outIntersection.position = (math::Vec4f(intersection.position.x(), intersection.position.y(), intersection.position.z(), 1.0f) * transform).head<3>();
								outIntersection.normal = (math::Vec4f(intersection.normal.x(), intersection.normal.y(), intersection.normal.z(), 0.0f) * transform).head<3>();
								outIntersection.t = (outIntersection.position - ray.origin).norm();

								outMatrixID = instance.modelToWorldID;

								objectID = getObjectID(modelIndex, meshIndex, materialIndex, instanceIndex);
							}
						}
					}
				}
			}
		}

		virtual unsigned int getObjectID(int modelIndex, int meshIndex, int materialIndex, int instanceIndex) const = 0;
		virtual bool getObjectByID(unsigned int objectID, PerModel*& outObjectModel, PerMesh*& outObjectMesh, PerMaterial*& outObjectMaterial, Instance*& outObjectInstance) = 0;
		virtual bool getObjectTransformID(unsigned int objectID, TransformSystem::ID& outID) = 0;
		virtual bool removeObjectByID(unsigned int objectID, PerModel& removedObject) = 0;
	protected:
		virtual void createInstanceBuffer(int totalInstances) = 0;
		virtual void setInstanceBufferForIA(ID3D11DeviceContext4* devcon) = 0;
		virtual void* getInstanceBufferMappedData() = 0;
		virtual void unmapInstanceBuffer() = 0;
		virtual void updateInstanceBufferData(const PerMesh& perMesh, void* instanceBufferData, int& copiedNum, const Mesh& mesh, const Camera& camera) = 0;
		virtual void initShader() = 0;
		virtual void bindMaterialData(const PerMaterial & perMaterial) = 0;

		void render(Shader& shader)
		{
			auto* devcon = D3D::getInstancePtr()->getDeviceContext();

			shader.bind();
			Renderer::getInstancePtr()->setPerFrameBuffersForVS();
			Renderer::getInstancePtr()->setPerFrameBuffersForGS();
			Renderer::getInstancePtr()->setPerFrameBuffersForPS();

			Renderer::getInstancePtr()->setPerViewBuffersForVS();
			Renderer::getInstancePtr()->setPerViewBuffersForGS();
			Renderer::getInstancePtr()->setPerViewBuffersForPS();

			Renderer::getInstancePtr()->setPerFrameGlobalSamplersForPS();

			LightSystem::getInstancePtr()->setPerFrameBufferForVS(devcon);
			LightSystem::getInstancePtr()->setPerFrameBufferForPS(devcon);


			setInstanceBufferForIA(devcon);
			int renderedInstances = 0;
			for (const auto& perModel : perModel)
			{
				if (!perModel.model)
				{
					_DEBUG_OUTPUT("Model is nullptr!")
						continue;
				}

				perModel.model->m_vertices.setVertexBufferForInputAssembler(devcon);
				perModel.model->m_indices.setIndexBufferForInputAssembler(devcon);

				for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
				{
					const Mesh& mesh = perModel.model->m_meshes[meshIndex];
					const auto& meshRange = perModel.model->m_ranges[meshIndex];

					for (const auto& perMaterial : perModel.perMesh[meshIndex].perMaterial)
					{
						if (perMaterial.instances.empty())
						{
							continue;
						}

						bindMaterialData(perMaterial);

						unsigned int numInstances = static_cast<unsigned int>(perMaterial.instances.size());
						if (perModel.model->m_indices.isEmpty())
						{
							D3D::getInstancePtr()->getDeviceContext()->DrawInstanced(meshRange.vertexNum, numInstances, meshRange.vertexOffset, renderedInstances);
						}
						else
						{
							D3D::getInstancePtr()->getDeviceContext()->DrawIndexedInstanced(meshRange.indexNum, numInstances, meshRange.indexOffset, meshRange.vertexOffset, renderedInstances);
						}
						renderedInstances += numInstances;
					}
				}
			}
		}

		std::vector<PerModel> perModel;

		Shader shader;
		Shader depth2DShader;
		Shader depthCubemapShader;
		struct PerDepthCubemapData
		{
			int32_t index;
			int32_t pad[3];
		};
		Buffer<PerDepthCubemapData> depthCubemapCBuffer;

		bool m_isNormalVisualizationOn = false;
		Shader lineNormalVisualization;

		Shader stencilShader;
		Shader GBufferGeometryShader;
	};
}
