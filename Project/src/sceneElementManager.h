#pragma once
#include "render/meshSystem/meshSystem.h"
#include "render/lightSystem/lightSystem.h"
#include "resourcesManagers/modelManager.h"



class SceneElementManager
{
public:
	Engine::HologramInstances::PerModel addHologramModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::HologramMaterial > material, const Engine::math::Mat4f& instanceTransform, const Engine::math::Vec3f& instanceColor);
	Engine::NormalVisInstances::PerModel addNormalVisModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr< Engine::ShadingGroupsDetails::NormalVisMaterial > material, const Engine::math::Mat4f& instanceTransform);
	Engine::TextureOnlyInstances::PerModel addTextureOnlyModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::TextureOnlyMaterial > material, const Engine::math::Mat4f& instanceTransform);
	Engine::LitInstances::PerModel addLitModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::LitMaterial > material, const Engine::math::Mat4f& instanceTransform);
	Engine::DissolutionInstances::PerModel addDissolutionModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::DissolutionMaterial > material, const Engine::math::Mat4f& instanceTransform);
	Engine::IncinerationInstances::PerModel addIncinerationModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr<Engine::ShadingGroupsDetails::IncinerationMaterial > material, const Engine::math::Mat4f& instanceTransform, const Engine::math::Vec3f& instnceSpherePosition);
	Engine::EmissionOnlyInstances::PerModel addEmissionOnlyModelElement(std::shared_ptr<Engine::Model> model, std::shared_ptr< Engine::ShadingGroupsDetails::EmissionOnlyMaterial > material, Engine::ShadingGroupsDetails::EmissionOnlyInstance instance);
	
	void setDirectionalLight(const Engine::math::Vec3f& energy, const Engine::math::Vec3f& direction, float solidAngle, float perceivedRadius, const Engine::Camera& mainCamera);
	void addPointLight(const Engine::math::Vec3f& energy, const Engine::math::Vec3f& position, float radius, bool withVisualization);
	void setSpotLight(const Engine::math::Vec3f& energy, const Engine::math::Mat4f& transform, float angle,std::shared_ptr<Engine::Texture> maskTexture, float radius);
	void setSpotLightTransform(const Engine::math::Mat4f& transform);

	void addSmokeParticleEmitter(const Engine::math::Vec3f& emitterPosition, float emitterSpawnRate, float emitterSpawnRadius, const Engine::math::Vec3f& emitterParticlesColor, bool withSphereVisualization);
};

