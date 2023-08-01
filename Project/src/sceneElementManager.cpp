#include "sceneElementManager.h"
#include "render/particleSystem/particleSystem.h"

Engine::HologramInstances::PerModel SceneElementManager::addHologramModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr < Engine::ShadingGroupsDetails::HologramMaterial> material, const Engine::math::Mat4f& instanceTransform, const Engine::math::Vec3f& instanceColor)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::HologramInstance instance = { id, instanceColor };
	return Engine::MeshSystem::getInstancePtr()->addHologramInstance(model, material, instance);
}

Engine::NormalVisInstances::PerModel SceneElementManager::addNormalVisModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr < Engine::ShadingGroupsDetails::NormalVisMaterial> material, const Engine::math::Mat4f& instanceTransform)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::NormalVisInstance instance = { id };
	return Engine::MeshSystem::getInstancePtr()->addNormalVisInstance(model, material, instance);
}

Engine::TextureOnlyInstances::PerModel SceneElementManager::addTextureOnlyModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr < Engine::ShadingGroupsDetails::TextureOnlyMaterial> material, const Engine::math::Mat4f& instanceTransform)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::TextureOnlyInstance instance = { id };
	return Engine::MeshSystem::getInstancePtr()->addTextureOnlyInstance(model, material, instance);
}

Engine::LitInstances::PerModel SceneElementManager::addLitModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr < Engine::ShadingGroupsDetails::LitMaterial> material, const Engine::math::Mat4f& instanceTransform)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::LitInstance instance = { id };

	return Engine::MeshSystem::getInstancePtr()->addLitInstance(model, material, instance);
}

Engine::DissolutionInstances::PerModel SceneElementManager::addDissolutionModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::DissolutionMaterial> material, const Engine::math::Mat4f& instanceTransform)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::DissolutionInstance instance = { id, 0.0f };

	return Engine::MeshSystem::getInstancePtr()->addDissolutionInstance(model, material, instance);
}

Engine::IncinerationInstances::PerModel SceneElementManager::addIncinerationModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::IncinerationMaterial> material, const Engine::math::Mat4f& instanceTransform, const Engine::math::Vec3f& instnceSpherePosition)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	mat = instanceTransform;

	Engine::ShadingGroupsDetails::IncinerationInstance instance;
	instance.modelToWorldID = id;
	instance.spherePosition = instnceSpherePosition;
	instance.sphereBigRadius = instance.sphereSmallRadius = 0.0f;
	instance.objectID = 0;

	return Engine::MeshSystem::getInstancePtr()->addIncinerationInstance(model, material, instance);
}

Engine::EmissionOnlyInstances::PerModel SceneElementManager::addEmissionOnlyModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr < Engine::ShadingGroupsDetails::EmissionOnlyMaterial> material, Engine::ShadingGroupsDetails::EmissionOnlyInstance instance)
{
	return Engine::MeshSystem::getInstancePtr()->addEmissionOnlyInstance(model, material, instance);
}

void SceneElementManager::setDirectionalLight(const Engine::math::Vec3f& energy, const Engine::math::Vec3f& direction, float solidAngle, float perceivedRadius, const Engine::Camera& mainCamera)
{
	Engine::LightSystem::getInstancePtr()->setDirectionalLight(energy, direction, solidAngle, perceivedRadius, mainCamera);
}

void SceneElementManager::addPointLight(const Engine::math::Vec3f& energy, const Engine::math::Vec3f& position, float radius, bool withVisualization)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();

	auto id = transformSystem->createMatrix();
	auto& mat = transformSystem->getMatrix(id);

	Engine::math::setTranslation(mat, position);

	Engine::LightSystem::getInstancePtr()->addPointLight({ 0.0f, 0.0f, 0.0f }, energy, id, radius);

	if (withVisualization)
	{
		auto sphereModel = Engine::ModelManager::getInstancePtr()->getUnitSphereModel();

		Engine::math::setScale(mat, Engine::math::Vec3f(radius, radius, radius));

		Engine::ShadingGroupsDetails::EmissionOnlyInstance ins = { id, energy };
		Engine::MeshSystem::getInstancePtr()->addEmissionOnlyInstance(sphereModel, std::make_shared<Engine::ShadingGroupsDetails::EmissionOnlyMaterial>(), ins);
	}
}

void SceneElementManager::setSpotLight(const Engine::math::Vec3f& energy, const Engine::math::Mat4f& transform, float angle, std::shared_ptr<Engine::Texture> maskTexture, float radius)
{
	Engine::LightSystem::getInstancePtr()->setSpotLight(energy, transform, angle, maskTexture, radius);
}

void SceneElementManager::setSpotLightTransform(const Engine::math::Mat4f& transform)
{
	Engine::LightSystem::getInstancePtr()->setSpotLightTransform(transform);
}

void SceneElementManager::addSmokeParticleEmitter(const Engine::math::Vec3f& emitterPosition, float emitterSpawnRate, float emitterSpawnRadius, const Engine::math::Vec3f& emitterParticlesColor, bool withSphereVisualization)
{
	auto* transformSystem = Engine::TransformSystem::getInstance();
	auto transformID = transformSystem->createMatrix();
	Engine::math::Mat4f& transformMatrix = transformSystem->getMatrix(transformID);
	Engine::math::setTranslation(transformMatrix, emitterPosition);

	Engine::ParticleSystem::getInstance()->addSmokeEmitter(transformID, emitterSpawnRate, emitterSpawnRadius, emitterParticlesColor);

	if (withSphereVisualization)
	{
		auto sphereModel = Engine::ModelManager::getInstancePtr()->getUnitSphereModel();
		
		Engine::math::Mat4f mat = Engine::math::Mat4f::Identity();
		Engine::math::setScale(transformMatrix, Engine::math::Vec3f(0.1f, 0.1f, 0.1f));
		
		Engine::ShadingGroupsDetails::EmissionOnlyInstance ins = { transformID, emitterParticlesColor };
		Engine::MeshSystem::getInstancePtr()->addEmissionOnlyInstance(sphereModel, std::make_shared<Engine::ShadingGroupsDetails::EmissionOnlyMaterial>(), ins);
	}
}
