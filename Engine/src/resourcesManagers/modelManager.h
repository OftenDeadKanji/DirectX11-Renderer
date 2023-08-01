#pragma once
#include <string>
#include <unordered_map>
#include "../render/meshSystem/mesh/model.h"
#include "../utils/nonCopyable.h"

namespace Engine
{
	class ModelManager
		: public NonCopyable
	{
	public:
		static ModelManager* createInstance();
		static void deleteInstance();
		static ModelManager* getInstancePtr();

		void deinit();

		std::shared_ptr<Model> getModel(const std::string& filePath);
		std::shared_ptr<Model> getUnitSphereModel();

	private:
		ModelManager() = default;
		static ModelManager* s_instance;

		std::unordered_map<std::string, std::shared_ptr<Model>> m_models;

		std::unordered_map<std::string, std::shared_ptr<Model>> m_basicShapesModels;
		const std::string UNIT_SPHERE_MODEL_NAME{ "UnitSphere" };

		std::shared_ptr<Model> loadModel(const std::string& filePath);
		std::shared_ptr<Model> createUnitSphereModel();

		void deleteAllModels();
	};
}
