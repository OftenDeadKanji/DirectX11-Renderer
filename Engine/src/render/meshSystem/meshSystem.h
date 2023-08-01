#pragma once
#include "ShadingGroups/hologramInstances.h"
#include "../camera/camera.h"
#include "../../objectMover/IObjectMover.h"
#include "../../math/intersection.h"
#include "ShadingGroups/normalVisInstances.h"
#include "ShadingGroups/textureOnlyInstances.h"
#include "ShadingGroups/litInstances.h"
#include "ShadingGroups/shadingGroup.h"
#include "ShadingGroups/emissionOnlyInstances.h"
#include "ShadingGroups/dissolutionInstances.h"
#include "ShadingGroups/incinerationInstances.h"
#include "../../utils/nonCopyable.h"

namespace Engine
{
	class MeshSystem
		: public NonCopyable
	{
	public:
		struct MeshIntersectionQuery
		{
			math::Intersection nearest;
			unsigned int objectID;
			std::unique_ptr<IObjectMover>* mover;
		};

	public:
		static MeshSystem* createInstance();
		static void deleteInstance();
		static MeshSystem* getInstancePtr();

		void deinit();

		void render()
		{
			Renderer::getInstancePtr()->disableBlending();
			m_normalVisInstances.render();
			m_textureOnlyInstances.render();
			m_hologramInstances.render();
			m_emissionOnlyInstances.render();

			m_litInstances.render();
			
			Renderer::getInstancePtr()->enableAlphaToCoverage();
			m_dissolutionInstances.render();

			Renderer::getInstancePtr()->switchToDepthEnabledStencilDisabledState();
		}

		void renderDepth2D()
		{
			Renderer::getInstancePtr()->disableBlending();
			m_litInstances.bindDepth2DShader();

			m_litInstances.renderDepth2D();
			m_normalVisInstances.renderDepth2D();
			m_textureOnlyInstances.renderDepth2D();
			m_emissionOnlyInstances.renderDepth2D();

			m_hologramInstances.bindDepth2DShader();
			m_hologramInstances.renderDepth2D();

			Renderer::getInstancePtr()->enableAlphaToCoverage();
			m_dissolutionInstances.bindDepth2DShader();
			m_dissolutionInstances.renderDepth2D();

			m_incinerationInstances.bindDepth2DShader();
			m_incinerationInstances.renderDepth2D();
		}

		void renderDepthCubemaps(const std::vector<math::Vec3f>& positions)
		{
			Renderer::getInstancePtr()->disableBlending();
			m_litInstances.bindDepthCubemapShader();
			
			m_litInstances.renderDepthCubemaps(positions);
			m_normalVisInstances.renderDepthCubemaps(positions);
			m_textureOnlyInstances.renderDepthCubemaps(positions);
			m_emissionOnlyInstances.renderDepthCubemaps(positions);

			m_hologramInstances.bindDepthCubemapShader();
			m_hologramInstances.renderDepthCubemaps(positions);

			Renderer::getInstancePtr()->enableAlphaToCoverage();
			m_dissolutionInstances.bindDepthCubemapShader();
			m_dissolutionInstances.renderDepthCubemaps(positions);

			m_incinerationInstances.bindDepthCubemapShader();
			m_incinerationInstances.renderDepthCubemaps(positions);
		}
	
		void renderStencil()
		{
			Renderer::getInstancePtr()->disableBlending();
			Renderer::getInstancePtr()->setReadWriteStencilRefValue(2);
			m_normalVisInstances.renderStencil();
			m_textureOnlyInstances.renderStencil();
			m_hologramInstances.renderStencil();
			m_emissionOnlyInstances.renderStencil();

			Renderer::getInstancePtr()->setReadWriteStencilRefValue(1);
			m_litInstances.renderStencil();

			Renderer::getInstancePtr()->enableAlphaToCoverage();
			m_dissolutionInstances.renderStencil();
			m_incinerationInstances.renderStencil();

			Renderer::getInstancePtr()->switchToDepthEnabledStencilDisabledState();
		}

		void renderGBufferGeometry()
		{
			Renderer::getInstancePtr()->switchToGBufferBlending();
			Renderer::getInstancePtr()->setReadWriteStencilRefValue(2);
			m_normalVisInstances.renderGBufferGeometry();
			m_textureOnlyInstances.renderGBufferGeometry();
			m_hologramInstances.renderGBufferGeometry();
			m_emissionOnlyInstances.renderGBufferGeometry();

			Renderer::getInstancePtr()->setReadWriteStencilRefValue(1);
			m_litInstances.renderGBufferGeometry();

			Renderer::getInstancePtr()->switchToGBufferBlendingWithAlphaToCoverage();
			m_dissolutionInstances.createAndBindGBufferNormalCopy();
			m_dissolutionInstances.renderGBufferGeometry();

			m_incinerationInstances.createAndBindGBufferNormalCopy();
			m_incinerationInstances.renderGBufferGeometry();

			Renderer::getInstancePtr()->switchToGBufferBlending();
		}

		HologramInstances::PerModel addHologramInstance(std::shared_ptr<Model> model, std::shared_ptr<ShadingGroupsDetails::HologramMaterial> material, ShadingGroupsDetails::HologramInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_hologramInstances.add(model, material, instance);
		}
		void addHologramInstance(HologramInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_hologramInstances.add(perModel);
		}
		//void removeObjectByID(unsigned int objectID);

		NormalVisInstances::PerModel addNormalVisInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::NormalVisMaterial> material, ShadingGroupsDetails::NormalVisInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_normalVisInstances.add(model, material, instance);
		}
		void addNormalVisInstance(NormalVisInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_normalVisInstances.add(perModel);
		}
		TextureOnlyInstances::PerModel addTextureOnlyInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::TextureOnlyMaterial> material, ShadingGroupsDetails::TextureOnlyInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_textureOnlyInstances.add(model, material, instance);
		}
		void addTextureOnlyInstance(TextureOnlyInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_textureOnlyInstances.add(perModel);
		}
		LitInstances::PerModel addLitInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::LitMaterial> material, ShadingGroupsDetails::LitInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_litInstances.add(model, material, instance);
		}
		void addLitInstance(LitInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}
			m_litInstances.add(perModel);
		}

		bool removeLitInstance(unsigned int objectID, LitInstances::PerModel& removedObject)
		{
			return m_litInstances.removeObjectByID(objectID, removedObject);
		}
		EmissionOnlyInstances::PerModel addEmissionOnlyInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::EmissionOnlyMaterial> material, ShadingGroupsDetails::EmissionOnlyInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_emissionOnlyInstances.add(model, material, instance);
		}
		void addEmissionOnlyInstance(EmissionOnlyInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_emissionOnlyInstances.add(perModel);
		}
		DissolutionInstances::PerModel addDissolutionInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::DissolutionMaterial> material, ShadingGroupsDetails::DissolutionInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_dissolutionInstances.add(model, material, instance);
		}
		void addDissolutionInstance(DissolutionInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_dissolutionInstances.add(perModel);
		}

		IncinerationInstances::PerModel addIncinerationInstance(std::shared_ptr<Model> model, std::shared_ptr < ShadingGroupsDetails::IncinerationMaterial> material, ShadingGroupsDetails::IncinerationInstance instance)
		{
			instance.objectID = ++instanceCounter;
			return m_incinerationInstances.add(model, material, instance);
		}
		void addIncinerationInstance(IncinerationInstances::PerModel& perModel)
		{
			unsigned int objectID = ++instanceCounter;

			for (auto& mesh : perModel.perMesh)
			{
				for (auto& material : mesh.perMaterial)
				{
					for (auto& instance : material.instances)
					{
						instance.objectID = objectID;
					}
				}
			}

			m_incinerationInstances.add(perModel);
		}

		void removeObjectByID(unsigned int objectID);
		void updateShadingGroupsInstanceBuffers(Camera& camera);

		bool findIntersection(const math::Ray& ray, MeshIntersectionQuery& intersection);

		void setNormalVisualization(bool state);

		TransformSystem::ID getObjectTransformID(unsigned int objectID);

		DissolutionInstances& getDissolutionInstances()
		{
			return m_dissolutionInstances;
		}
		IncinerationInstances& getIncinerationInstances()
		{
			return m_incinerationInstances;
		}
	private:
		MeshSystem() = default;

		static MeshSystem* s_instance;

		HologramInstances m_hologramInstances;
		NormalVisInstances m_normalVisInstances;
		TextureOnlyInstances m_textureOnlyInstances;
		LitInstances m_litInstances;
		EmissionOnlyInstances m_emissionOnlyInstances;
		DissolutionInstances m_dissolutionInstances;
		IncinerationInstances m_incinerationInstances;

		unsigned int instanceCounter = 0;

		void findIntersectionInternal(const math::Ray& ray, TransformSystem::ID& outMatrixID, math::Intersection& outNearest, int& instanceNumber, unsigned int& objectID);
		void deleteAllInstances();
	};
}
