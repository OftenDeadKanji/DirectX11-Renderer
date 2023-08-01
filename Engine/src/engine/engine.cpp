#include "engine.h"
#include "../render/Direct3d/d3d.h"
#include "../render/meshSystem/meshSystem.h"
#include "../render/lightSystem/lightSystem.h"
#include "../resourcesManagers/modelManager.h"
#include "../resourcesManagers/textureManager.h"
#include "../transformSystem/transformSystem.h"
#include "renderer.h"
#include "../utils/random/random.h"
#include "../render/particleSystem/particleSystem.h"
#include "../render/decalSystem/decalSystem.h"

namespace Engine
{
	void Engine::init()
	{
		D3D::createInstance()->init();
		Random::createInstance()->init();
		ModelManager::createInstance();
		TextureManager::createInstance()->init();
		MeshSystem::createInstance();
		LightSystem::createInstance();
		TransformSystem::createInstance();
		Renderer::createInstance();
		ParticleSystem::createInstance()->init();
		DecalSystem::createInstance()->init();
	}

	void Engine::deinit()
	{
		DecalSystem::deleteInstance();
		Random::deleteInstance();
		ParticleSystem::deleteInstance();
		Renderer::deleteInstance();
		TransformSystem::deleteInstance();
		LightSystem::deleteInstance();
		MeshSystem::deleteInstance();
		TextureManager::deleteInstance();
		ModelManager::deleteInstance();
		D3D::deleteInstance();
	}
}