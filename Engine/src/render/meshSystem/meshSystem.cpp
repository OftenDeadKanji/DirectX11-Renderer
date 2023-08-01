#include "meshSystem.h"
#include "../../math/intersection.h"
#include "../../objectMover/matrixMover.h"

namespace Engine
{
	MeshSystem* MeshSystem::s_instance = nullptr;

	MeshSystem* MeshSystem::createInstance()
	{
		if (!s_instance)
		{
			s_instance = new MeshSystem();
		}

		return s_instance;
	}

	void MeshSystem::deleteInstance()
	{
		s_instance->deinit();
		delete s_instance;

		s_instance = nullptr;
	}

	MeshSystem* MeshSystem::getInstancePtr()
	{
		return s_instance;
	}

	void MeshSystem::deinit()
	{
		deleteAllInstances();
	}

	bool MeshSystem::findIntersection(const math::Ray& ray, MeshIntersectionQuery& outIntersection)
	{
		TransformSystem::ID matID;
		int instanceNumber = -1;

		findIntersectionInternal(ray, matID, outIntersection.nearest, instanceNumber, outIntersection.objectID);
		if (outIntersection.nearest.valid())
		{
			if (outIntersection.mover)
			{
				outIntersection.mover->reset(new MatrixMover(matID));
			}

			return true;
		}

		return false;
	}

	void MeshSystem::setNormalVisualization(bool state)
	{
		m_hologramInstances.setNormalVisualization(state);
		m_normalVisInstances.setNormalVisualization(state);
		m_textureOnlyInstances.setNormalVisualization(state);
		m_litInstances.setNormalVisualization(state);
		m_emissionOnlyInstances.setNormalVisualization(state);
		m_dissolutionInstances.setNormalVisualization(state);
	}

	TransformSystem::ID MeshSystem::getObjectTransformID(unsigned int objectID)
	{
		TransformSystem::ID id = 0;

		if (m_hologramInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}
		if (m_normalVisInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}
		if (m_textureOnlyInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}
		if (m_litInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}
		if (m_emissionOnlyInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}
		if (m_dissolutionInstances.getObjectTransformID(objectID, id))
		{
			return id;
		}

		return id;
	}

	void MeshSystem::deleteAllInstances()
	{
		m_hologramInstances.clear();
		m_normalVisInstances.clear();
		m_textureOnlyInstances.clear();
		m_litInstances.clear();
		m_emissionOnlyInstances.clear();
		m_dissolutionInstances.clear();
	}

	void MeshSystem::removeObjectByID(unsigned int objectID)
	{
		HologramInstances::PerModel hologramPerModel;
		if (m_hologramInstances.removeObjectByID(objectID, hologramPerModel))
		{
			return;
		}

		NormalVisInstances::PerModel normalVisPerModel;
		if (m_normalVisInstances.removeObjectByID(objectID, normalVisPerModel))
		{
			return;
		}

		TextureOnlyInstances::PerModel textureOnlyPerModel;
		if (m_textureOnlyInstances.removeObjectByID(objectID, textureOnlyPerModel))
		{
			return;
		}

		LitInstances::PerModel litPerModel;
		if (m_litInstances.removeObjectByID(objectID, litPerModel))
		{
			return;
		}

		EmissionOnlyInstances::PerModel emissionOnlyPerModel;
		if (m_emissionOnlyInstances.removeObjectByID(objectID, emissionOnlyPerModel))
		{
			return;
		}

		DissolutionInstances::PerModel dissolutionyPerModel;
		if (m_dissolutionInstances.removeObjectByID(objectID, dissolutionyPerModel))
		{
			return;
		}
	}

	void MeshSystem::updateShadingGroupsInstanceBuffers(Camera& camera)
	{
		m_hologramInstances.updateInstanceBuffers(camera);
		m_normalVisInstances.updateInstanceBuffers(camera);
		m_textureOnlyInstances.updateInstanceBuffers(camera);
		m_litInstances.updateInstanceBuffers(camera);
		m_emissionOnlyInstances.updateInstanceBuffers(camera);
		m_dissolutionInstances.updateInstanceBuffers(camera);
		m_incinerationInstances.updateInstanceBuffers(camera);
	}

	void MeshSystem::findIntersectionInternal(const math::Ray& ray, TransformSystem::ID& outMatrixID, math::Intersection& outNearest, int& instanceNumber, unsigned int& objectID)
	{
		m_hologramInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
		m_normalVisInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
		m_textureOnlyInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
		m_litInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
		m_emissionOnlyInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
		m_dissolutionInstances.findIntersection(ray, outNearest, outMatrixID, objectID);
	}
}