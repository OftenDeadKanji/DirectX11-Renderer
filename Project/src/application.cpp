#include "application.h"
#include "dependencies/Windows/win.h"
#include <thread>
#include <unordered_set>
#include <set>
#include "math/ray.h"
#include "engine/engine.h"
#include "engine/renderer.h"

#include "render/meshSystem/meshSystem.h"
#include "render/lightSystem/lightSystem.h"
#include "resourcesManagers/modelManager.h"
#include "resourcesManagers/textureManager.h"

#include "render/particleSystem/particleSystem.h"
#include "render/decalSystem/decalSystem.h"

#include "utils/random/random.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

EventDispatcher Application::eventDispatcher{};

inline bool isKeyPressed(int key)
{
	return GetKeyState(key) & 0x8000;
}

Application::Application(HWND windowHandle)
	: m_timer(60), m_window(windowHandle, 0.5f)
{
	Engine::Renderer::getInstancePtr()->init(m_window.getBackbufferTex());
	Engine::Renderer::getInstancePtr()->setClearColor(Engine::math::Vec3f(0.0f, 0.0f, 0.0f));
	Engine::Renderer::getInstancePtr()->setEV100(m_ev100);

	Application::eventDispatcher.addWindowListener(this);

	initCamera();

	auto* textureManager = Engine::TextureManager::getInstance();
	{
		auto tex = textureManager->getTexture(L"Assets/Textures/Cubemaps/Cubemap_mountains.dds");
		Engine::Renderer::getInstancePtr()->setSkyTexture(tex);
	}
	Engine::Renderer::getInstancePtr()->prerenderIBLTextures(L"Assets/Textures/IBL/diff.dds", L"Assets/Textures/IBL/spec.dds", L"Assets/Textures/IBL/refl.dds");
	{
		auto diff = textureManager->getTexture(L"Assets/Textures/IBL/diff.dds");
		auto spec = textureManager->getTexture(L"Assets/Textures/IBL/spec.dds");
		auto fac = textureManager->getTexture(L"Assets/Textures/IBL/refl.dds");

		Engine::Renderer::getInstancePtr()->setIBLTextures(diff, spec, fac);
	}

	createLights();
	createLitObjects();
	createTextureOnlyObjects();
	createHologramObjects();
	createParticles();
	
	{
		auto tex = textureManager->getTexture(L"Assets/Textures/2D/Decal_splatter.dds");
		Engine::DecalSystem::getInstance()->setTexture(tex);
	}
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(windowHandle);
	ImGui_ImplDX11_Init(Engine::D3D::getInstancePtr()->getDevice(), Engine::D3D::getInstancePtr()->getDeviceContext());

	m_timer.newFrameStarted();
}

Application::~Application()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Application::createLights()
{
	m_sceneElementManager.setDirectionalLight(Engine::math::Vec3f(10000.0f, 10000.0f, 10000.0f), Engine::math::Vec3f(0.0f, -0.2f, 1.0f).normalized(), 0.000067f, 0.01f, m_camera);

	m_sceneElementManager.addPointLight(Engine::math::Vec3f(9.0f, 2.0f, 2.0f), Engine::math::Vec3f(-4.5f, 2.0f, 2.0f), 0.2f, true);
	m_sceneElementManager.addPointLight(Engine::math::Vec3f(4.0f, 9.0f, 2.0f), Engine::math::Vec3f(-2.5f, 2.0f, 2.0f), 0.2f, true);
	m_sceneElementManager.addPointLight(Engine::math::Vec3f(0.0f, 5.0f, 9.0f), Engine::math::Vec3f(-1.0f, 2.0f, 2.4f), 0.2f, true);
	m_sceneElementManager.addPointLight(Engine::math::Vec3f(4.0f, 9.0f, 2.0f), Engine::math::Vec3f(1.0f, 2.0f, 2.4f), 0.2f, true);
	m_sceneElementManager.addPointLight(Engine::math::Vec3f(9.0f, 5.0f, 9.0f), Engine::math::Vec3f(2.5f, 2.0f, 2.0f), 0.2f, true);
	m_sceneElementManager.addPointLight(Engine::math::Vec3f(9.0f, 8.0f, 0.0f), Engine::math::Vec3f(4.5f, 2.0f, 2.0f), 0.2f, true);
	
	auto batSignalTex = Engine::TextureManager::getInstance()->getTexture(L"Assets/Textures/2D/batSignal.dds");
	m_sceneElementManager.setSpotLight(Engine::math::Vec3f(200.0f, 200.0f, 200.0f), m_camera.getViewInv(), 7.5f, batSignalTex, 0.1f);
}

void Application::createLitObjects()
{
	using namespace Engine::ShadingGroupsDetails;
	using namespace Engine::math;

	auto* textureManager = Engine::TextureManager::getInstance();
	auto* modelManager = Engine::ModelManager::getInstancePtr();

	{
		auto eastTower = modelManager->getModel("Assets/Models/EastTower/EastTower.fbx");

		Mat4f instanceMat = Engine::math::Mat4f::Identity();
		auto litMat = std::make_shared<LitMaterial>();
		litMat->name = "default_eastTower";
		Engine::math::setTranslation(instanceMat, Vec3f(0.0f, 0.0f, 3.0f));
		auto added = m_sceneElementManager.addLitModelElement(eastTower, litMat, instanceMat);

		added.perMesh[0].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/EastTower/dds/CityWalls_Diffuse.dds");
		added.perMesh[0].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/EastTower/dds/CityWalls_Normal.dds");
		added.perMesh[0].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/EastTower/dds/CityWalls_ARM.dds");

		added.perMesh[1].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/EastTower/dds/PalaceWall_Diffuse.dds");

		added.perMesh[2].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/EastTower/dds/Trims_Diffuse.dds");
		added.perMesh[2].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/EastTower/dds/Trims_Normal.dds");
		added.perMesh[2].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/EastTower/dds/Trims_ARM.dds");

		added.perMesh[3].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/EastTower/dds/Statue_Diffuse.dds");
		added.perMesh[3].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/EastTower/dds/Statue_Normal.dds");
		added.perMesh[3].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/EastTower/dds/Statue_ARM.dds");

		added.perMesh[4].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/EastTower/dds/StoneWork_Diffuse.dds");
		added.perMesh[4].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/EastTower/dds/StoneWork_Normal.dds");
		added.perMesh[4].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/EastTower/dds/StoneWork_ARM.dds");
	}
	{
		auto knight = modelManager->getModel("Assets/Models/Knight/Knight.fbx");

		Mat4f instanceMat = Mat4f::Identity();
		auto litMat = std::make_shared<LitMaterial>();
		litMat->name = "default_knight";
		Engine::math::setTranslation(instanceMat, Vec3f(1.0f, 0.0f, 0.5f));
		auto added = m_sceneElementManager.addLitModelElement(knight, litMat, instanceMat);

		added.perMesh[0].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Fur_Diffuse.dds");
		added.perMesh[0].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Fur_Normal.dds");
		added.perMesh[0].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Fur_ARM.dds");

		added.perMesh[1].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Pants_Diffuse.dds");
		added.perMesh[1].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Pants_Normal.dds");
		added.perMesh[1].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Pants_ARM.dds");

		added.perMesh[2].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Torso_Diffuse.dds");
		added.perMesh[2].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Torso_Normal.dds");
		added.perMesh[2].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Torso_ARM.dds");

		added.perMesh[3].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Head_Diffuse.dds");
		added.perMesh[3].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Head_Normal.dds");
		added.perMesh[3].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Head_ARM.dds");

		added.perMesh[4].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Eyes_Diffuse.dds");
		added.perMesh[4].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Eyes_Normal.dds");

		added.perMesh[5].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Helmet_Diffuse.dds");
		added.perMesh[5].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Helmet_Normal.dds");
		added.perMesh[5].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Helmet_ARM.dds");

		added.perMesh[6].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Skirt_Diffuse.dds");
		added.perMesh[6].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Skirt_Normal.dds");
		added.perMesh[6].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Skirt_ARM.dds");

		added.perMesh[7].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Cloak_Diffuse.dds");
		added.perMesh[7].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Cloak_Normal.dds");
		added.perMesh[7].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Cloak_ARM.dds");

		added.perMesh[8].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Gloves_Diffuse.dds");
		added.perMesh[8].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Knight/dds/Gloves_Normal.dds");
		added.perMesh[8].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Knight/dds/Gloves_ARM.dds");
	}
	{
		auto knightHorse = modelManager->getModel("Assets/Models/KnightHorse/KnightHorse.fbx");

		Mat4f instanceMat = Mat4f::Identity();
		auto litMat = std::make_shared<LitMaterial>();
		litMat->name = "default_knightHorse";
		Engine::math::setTranslation(instanceMat, Vec3f(-1.0f, 0.0f, 0.5f));
		auto added = m_sceneElementManager.addLitModelElement(knightHorse, litMat, instanceMat);

		added.perMesh[0].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Armor_Diffuse.dds");
		added.perMesh[0].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Armor_Normal.dds");
		added.perMesh[0].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Armor_ARM.dds");

		added.perMesh[1].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Horse_Diffuse.dds");
		added.perMesh[1].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Horse_Normal.dds");
		added.perMesh[1].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Horse_ARM.dds");

		added.perMesh[2].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Tail_Diffuse.dds");
		added.perMesh[2].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/KnightHorse/dds/Tail_Normal.dds");
	}
	{
		auto cube = modelManager->getModel("Assets/Models/Cube/cube.fbx");

		{
			Mat4f instanceMat = Mat4f::Identity();
			setScale(instanceMat, Vec3f(0.8f, 0.8f, 0.8f));

			Engine::math::setTranslation(instanceMat, Vec3f(-4.5f, 2.0f, 3.0f));
			auto pavementMaterial = std::make_shared<LitMaterial>();
			pavementMaterial->name = "Pavement";
			pavementMaterial->textureDiffuse = textureManager->getTexture(L"Assets/Textures/2D/2D_pavement_diff.dds");
			pavementMaterial->textureNormal = textureManager->getTexture(L"Assets/Textures/2D/2D_pavement_normal.dds");
			pavementMaterial->textureARM = textureManager->getTexture(L"Assets/Textures/2D/2D_pavement_arm.dds");
			m_sceneElementManager.addLitModelElement(cube, pavementMaterial, instanceMat);

			Engine::math::setTranslation(instanceMat, Vec3f(-2.5f, 2.0f, 3.0f));
			auto rustyMetalMaterial = std::make_shared<LitMaterial>();
			rustyMetalMaterial->name = "RustyMetal";
			rustyMetalMaterial->textureDiffuse = textureManager->getTexture(L"Assets/Textures/2D/2D_metal_grate_rusty_diff.dds");
			rustyMetalMaterial->textureNormal = textureManager->getTexture(L"Assets/Textures/2D/2D_metal_grate_rusty_normal.dds");
			rustyMetalMaterial->textureARM = textureManager->getTexture(L"Assets/Textures/2D/2D_metal_grate_rusty_arm.dds");
			m_sceneElementManager.addLitModelElement(cube, rustyMetalMaterial, instanceMat);

			Engine::math::setTranslation(instanceMat, Vec3f(2.5f, 2.0f, 3.0f));
			auto woodMaterial = std::make_shared<LitMaterial>();
			woodMaterial->name = "Wood";
			woodMaterial->textureDiffuse = textureManager->getTexture(L"Assets/Textures/2D/2D_wood_diff.dds");
			woodMaterial->textureNormal = textureManager->getTexture(L"Assets/Textures/2D/2D_wood_normal.dds");
			woodMaterial->textureARM = textureManager->getTexture(L"Assets/Textures/2D/2D_wood_arm.dds");
			m_sceneElementManager.addLitModelElement(cube, woodMaterial, instanceMat);

			Engine::math::setTranslation(instanceMat, Vec3f(4.5f, 2.0f, 3.0f));
			auto brickMaterial = std::make_shared<LitMaterial>();
			brickMaterial->name = "Brick";
			brickMaterial->textureDiffuse = textureManager->getTexture(L"Assets/Textures/2D/2D_brick_diff.dds");
			brickMaterial->textureNormal = textureManager->getTexture(L"Assets/Textures/2D/2D_brick_normal.dds");
			brickMaterial->textureARM = textureManager->getTexture(L"Assets/Textures/2D/2D_brick_arm.dds");
			m_sceneElementManager.addLitModelElement(cube, brickMaterial, instanceMat);

		}

		{
			Mat4f instanceMat = Mat4f::Identity();
			setScale(instanceMat, Vec3f(1.0, 0.1f, 1.0f));

			auto mudRoadMaterial = std::make_shared<LitMaterial>();
			mudRoadMaterial->name = "MudRoad";
			mudRoadMaterial->textureDiffuse = textureManager->getTexture(L"Assets/Textures/2D/2D_mud_road_diff.dds");
			mudRoadMaterial->textureNormal = textureManager->getTexture(L"Assets/Textures/2D/2D_mud_road_normal.dds");
			mudRoadMaterial->textureARM = textureManager->getTexture(L"Assets/Textures/2D/2D_mud_road_arm.dds");
			for (int i = 0; i < 6; i++)
			{
				for (int j = 0; j < 5; j++)
				{
					Engine::math::setTranslation(instanceMat, Vec3f(-5.0f + i * 2, -0.1f, -3.0f + j * 2));
					m_sceneElementManager.addLitModelElement(cube, mudRoadMaterial, instanceMat);
				}
			}
		}
	}
}

void Application::createTextureOnlyObjects()
{
	using namespace Engine::ShadingGroupsDetails;
	using namespace Engine::math;

	{
		auto* modelManager = Engine::ModelManager::getInstancePtr();
		auto* textureManager = Engine::TextureManager::getInstance();

		auto knight = Engine::ModelManager::getInstancePtr()->getModel("Assets/Models/Knight/Knight.fbx");

		Mat4f instanceMat = Mat4f::Identity();
		auto texOnlyMat = std::make_shared<TextureOnlyMaterial>();
		Engine::math::setTranslation(instanceMat, Vec3f(1.0f, 0.0f, 0.5f));

		auto added = m_sceneElementManager.addTextureOnlyModelElement(knight, texOnlyMat, instanceMat);

		added.perMesh[0].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Fur_Diffuse.dds");
		added.perMesh[1].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Pants_Diffuse.dds");
		added.perMesh[2].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Torso_Diffuse.dds");
		added.perMesh[3].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Head_Diffuse.dds");
		added.perMesh[4].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Eyes_Diffuse.dds");
		added.perMesh[5].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Helmet_Diffuse.dds");
		added.perMesh[6].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Skirt_Diffuse.dds");
		added.perMesh[7].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Cloak_Diffuse.dds");
		added.perMesh[8].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Knight/dds/Gloves_Diffuse.dds");
	}
}

void Application::createHologramObjects()
{
	using namespace Engine::ShadingGroupsDetails;
	using namespace Engine::math;

	auto samurai = Engine::ModelManager::getInstancePtr()->getModel("Assets/Models/Samurai/Samurai.fbx");
	
	Mat4f transform = Mat4f::Identity();
	setTranslation(transform, { 0.0f, 0.0f, 1.0f });
	
	m_sceneElementManager.addHologramModelElement(samurai, std::make_shared<HologramMaterial>(), transform, Engine::math::Vec3f(1.0f, 1.0f, 1.0f));
}

void Application::createParticles()
{
	auto* textureManager = Engine::TextureManager::getInstance();

	auto smokeRTLTex = textureManager->getTexture(L"Assets/Textures/2D/smoke_RLT.dds");
	auto smokeBBFTex = textureManager->getTexture(L"Assets/Textures/2D/smoke_BBF.dds");
	auto smokeEMVATex = textureManager->getTexture(L"Assets/Textures/2D/smoke_EMVA.dds");

	auto* particleSystem = Engine::ParticleSystem::getInstance();
	particleSystem->setSmokeTexture(smokeRTLTex, smokeBBFTex, smokeEMVATex);

	m_sceneElementManager.addSmokeParticleEmitter({ -2.0f, 0.1f, -3.0f }, 3.0f, 0.2f, { 1.0f, 0.2f, 0.1f }, true);
	m_sceneElementManager.addSmokeParticleEmitter({ 2.0f, 0.1f, -3.0f }, 3.0f, 0.2f, { 1.0f, 0.9f, 0.1f }, true);
}

void Application::spawnDissolutionObject(Engine::math::Vec3f position)
{
	auto* modelManager = Engine::ModelManager::getInstancePtr();
	auto* textureManager = Engine::TextureManager::getInstance();

	auto samurai = modelManager->getModel("Assets/Models/Samurai/Samurai.fbx");
	auto noiseTex = textureManager->getTexture(L"Assets/Textures/2D/Noise_16.dds");

	auto dissolutionMat = std::make_shared<Engine::ShadingGroupsDetails::DissolutionMaterial>();
	dissolutionMat->name = "default_samurai";
	dissolutionMat->textureNoise = noiseTex;

	Engine::math::Mat4f instanceMat = Engine::math::Mat4f::Identity();
	Engine::math::setTranslation(instanceMat, position);

	auto added = m_sceneElementManager.addDissolutionModelElement(samurai, dissolutionMat, instanceMat);

	added.perMesh[0].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Sword_Diffuse.dds");
	added.perMesh[0].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Sword_Normal.dds");
	added.perMesh[0].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Sword_ARM.dds");

	added.perMesh[1].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Head_Diffuse.dds");
	added.perMesh[1].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Head_Normal.dds");
	added.perMesh[1].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Head_ARM.dds");

	added.perMesh[2].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Eyes_Diffuse.dds");
	added.perMesh[2].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Eyes_Normal.dds");
	added.perMesh[2].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Eyes_ARM.dds");

	added.perMesh[3].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Helmet_Diffuse.dds");
	added.perMesh[3].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Helmet_Normal.dds");
	added.perMesh[3].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Helmet_ARM.dds");

	added.perMesh[4].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_Diffuse.dds");
	added.perMesh[4].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_Normal.dds");
	added.perMesh[4].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_ARM.dds");

	added.perMesh[5].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Legs_Diffuse.dds");
	added.perMesh[5].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Legs_Normal.dds");
	added.perMesh[5].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Legs_ARM.dds");

	added.perMesh[6].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Hands_Diffuse.dds");
	added.perMesh[6].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Hands_Normal.dds");
	added.perMesh[6].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Hands_ARM.dds");

	added.perMesh[7].perMaterial.front().material->textureDiffuse = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_Diffuse.dds");
	added.perMesh[7].perMaterial.front().material->textureNormal = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_Normal.dds");
	added.perMesh[7].perMaterial.front().material->textureARM = textureManager->getTexture(L"Assets/Models/Samurai/dds/Torso_ARM.dds");

	added.perMesh[0].perMaterial.front().instances.front().time = 0.0f;
}

void Application::run()
{
	while (m_mainLoopCondition)
	{
		if (m_timer.frameElapsed())
		{
			updateTime();
			processInputs();

			update();

			Engine::Renderer::getInstancePtr()->update(m_camera, m_prevCamera, m_deltaTime, m_timeSinceStart);
			Engine::Renderer::getInstancePtr()->render(m_camera);

			processImGui();

			m_window.flush();
		}

		std::this_thread::yield();
	}
}

void Application::processInputs()
{
	MSG msg;

	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	m_prevCamera = m_camera;

	Engine::math::Vec3f camRelativeOffset(0.0f, 0.0f, 0.0f);
	Engine::math::Vec3f camRelativeAngles(0.0f, 0.0f, 0.0f);
	
	if (m_keyStates[VK_OEM_PLUS])
	{
		m_ev100 += m_ev100ChangeValue * m_deltaTime;
		Engine::Renderer::getInstancePtr()->setEV100(m_ev100);
	}
	if (m_keyStates[VK_OEM_MINUS])
	{
		m_ev100 -= m_ev100ChangeValue * m_deltaTime;
		Engine::Renderer::getInstancePtr()->setEV100(m_ev100);
	}

	if (m_keyStates['A'])
	{
		camRelativeOffset[0] -= m_cameraMovementSpeed;
	}
	if (m_keyStates['D'])
	{
		camRelativeOffset[0] += m_cameraMovementSpeed;
	}
	if (m_keyStates['Q'])
	{
		camRelativeOffset[1] -= m_cameraMovementSpeed;
	}
	if (m_keyStates['E'])
	{
		camRelativeOffset[1] += m_cameraMovementSpeed;
	}
	if (m_keyStates['S'])
	{
		camRelativeOffset[2] -= m_cameraMovementSpeed;
	}
	if (m_keyStates['W'])
	{
		camRelativeOffset[2] += m_cameraMovementSpeed;
	}

	if (m_keyStates[VK_LSHIFT])
	{
		camRelativeOffset *= m_cameraMovementSpeedup;
	}
	

	if (m_rotateCamera)
	{
		Engine::math::Vec2i mousePosDifference = m_cameraRotationMouseCurrentPos - m_cameraRotationMouseStartPos;

		camRelativeAngles[0] = (mousePosDifference.y() * Engine::math::deg2rad(m_cameraRotationSpeed)) / (0.5f * m_window.getSize().x());
		camRelativeAngles[1] = -(mousePosDifference.x() * Engine::math::deg2rad(m_cameraRotationSpeed)) / (0.5f * m_window.getSize().x());
	}

#if 1
	Engine::math::Vec4f prevMousePosWS =
	{
		2.0f * (static_cast<float>(m_mousePrevPos.x()) / m_window.getSize().x()) - 1.0f,
		2.0f * (static_cast<float>(m_mousePrevPos.y()) / m_window.getSize().y()) - 1.0f,
		m_objectZInClipSpace,
		1.0f
	};
	m_camera.transformFromClipToWorldSpace(prevMousePosWS);
#endif

	camRelativeAngles *= m_deltaTime;
	camRelativeOffset *= m_deltaTime;

	m_camera.addRelativeAngles(camRelativeAngles);
	m_camera.addRelativeOffset(camRelativeOffset);
	m_camera.updateCamera();

#if 1
	Engine::math::Vec4f newMousePosWS =
	{
		2.0f * (static_cast<float>(m_mousePrevPos.x()) / m_window.getSize().x()) - 1.0f,
		2.0f * (static_cast<float>(m_mousePrevPos.y()) / m_window.getSize().y()) - 1.0f,
		m_objectZInClipSpace,
		1.0f
	};
	m_camera.transformFromClipToWorldSpace(newMousePosWS);
	Engine::math::Vec4f diff = newMousePosWS - prevMousePosWS;

	if (m_sceneObjectMover)
	{
		m_sceneObjectMover->move(diff.head<3>());
	}
#endif

	camRelativeOffset = camRelativeAngles = Engine::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void Application::onWindowClose()
{
	m_mainLoopCondition = false;
}

void Application::onWindowResize(int width, int height)
{
	m_window.setSize(Engine::math::Vec2i(width, height));

	Engine::math::Vec2f windowSizef = { width, height };

	m_camera.setPerspective(45.0f, windowSizef.x() / windowSizef.y(), 0.1f, 1000.0f);
}

void Application::onKeyDown(wchar_t key)
{
	m_keyStates[key] = true;
	
	if (key == 'N')
	{
		Engine::math::Vec3f spawnPos = m_camera.position();
		spawnPos += m_camera.forward() * 2.0f - m_camera.top();

		spawnDissolutionObject(spawnPos);
	}
	else if (key == 'F')
	{
		isFlashlightAttachedToCamera = !isFlashlightAttachedToCamera;
	}
	else if (key == 'G')
	{
		Engine::math::Ray ray;
		ray.origin = m_camera.position();
		ray.direction = m_camera.forward();

		Engine::MeshSystem::MeshIntersectionQuery intersection;
		intersection.nearest.reset();
		intersection.mover = nullptr;
		if (Engine::MeshSystem::getInstancePtr()->findIntersection(ray, intersection))
		{
			Engine::DecalSystem::getInstance()->addInstance(intersection.nearest.position, ray.direction, intersection.objectID);
		}
	}
	else if (key == VK_DELETE)
	{
		Engine::math::Ray ray = m_camera.generateRay(m_mousePos, m_window.getSize());

		Engine::MeshSystem::MeshIntersectionQuery query;
		query.mover = nullptr;
		query.nearest.reset();

		Engine::MeshSystem::getInstancePtr()->findIntersection(ray, query);

		Engine::LitInstances::PerModel litInstance;
		if (Engine::MeshSystem::getInstancePtr()->removeLitInstance(query.objectID, litInstance))
		{
			Engine::IncinerationInstances::PerModel incinerationInstance;

			Engine::Random* random = Engine::Random::getInstance();
			Engine::math::Vec3f instanceParticleColor = Engine::math::Vec3f(random->getRandomFloat(), random->getRandomFloat(), random->getRandomFloat()).normalized();
			incinerationInstance.model = litInstance.model;

			int transformID = Engine::TransformSystem::getInstance()->createMatrix();
			for (int meshIndex = 0; meshIndex < litInstance.perMesh.size(); meshIndex++)
			{
				auto& perMesh = litInstance.perMesh[meshIndex];

				Engine::IncinerationInstances::PerMesh perMeshToAdd;
				for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
				{
					auto& perMaterial = perMesh.perMaterial[materialIndex];

					Engine::IncinerationInstances::PerMaterial perMaterialToAdd;
					perMaterialToAdd.material = std::make_shared< Engine::ShadingGroupsDetails::IncinerationMaterial>();
					perMaterialToAdd.material->name = perMaterial.material->name;
					perMaterialToAdd.material->textureDiffuse = perMaterial.material->textureDiffuse;
					perMaterialToAdd.material->textureNormal = perMaterial.material->textureNormal;
					perMaterialToAdd.material->textureARM = perMaterial.material->textureARM;
					perMaterialToAdd.material->useDefaultRoughness = perMaterial.material->useDefaultRoughness;
					perMaterialToAdd.material->useDefaultMetalness = perMaterial.material->useDefaultMetalness;
					perMaterialToAdd.material->textureNoise = Engine::TextureManager::getInstance()->getTexture(L"Assets/Textures/2D/Noise_16.dds");

					auto& instance = perMaterial.instances.front();

					Engine::ShadingGroupsDetails::IncinerationInstance instanceToAdd;
					instanceToAdd.modelToWorldID = transformID;
					instanceToAdd.spherePosition = query.nearest.position;
					instanceToAdd.sphereBigRadius = instanceToAdd.spherePrevioursBigRadius = instanceToAdd.sphereSmallRadius = 0.0f;
					
					instanceToAdd.particleColor = instanceParticleColor;

					auto& transform = Engine::TransformSystem::getInstance()->getMatrix(instanceToAdd.modelToWorldID);
					transform = Engine::TransformSystem::getInstance()->getMatrix(instance.modelToWorldID);

					perMaterialToAdd.instances.push_back(instanceToAdd);
					perMeshToAdd.perMaterial.push_back(perMaterialToAdd);
				}
				incinerationInstance.perMesh.push_back(perMeshToAdd);
			}

			Engine::MeshSystem::getInstancePtr()->addIncinerationInstance(incinerationInstance);
		}
	}
}

void Application::onKeyUp(wchar_t key)
{
	m_keyStates[key] = false;
}

void Application::onLeftMouseButtonDown(int x, int y)
{
	m_rotateCamera = true;
	m_cameraRotationMouseCurrentPos = m_cameraRotationMouseStartPos = { x, y };
}

void Application::onLeftMouseButtonUp(int x, int y)
{
	m_rotateCamera = false;
}

void Application::onMouseMove(int x, int y)
{
	m_mousePrevPos = m_mousePos;
	m_mousePos = { x, y };

	if (m_rotateCamera)
	{
		m_cameraRotationMouseCurrentPos = { x, y };
	}
#if 1
	if (m_sceneObjectMover)
	{
		Engine::math::Vec2i winSize = m_window.getSize();

		Engine::math::Vec4f currentPos =
		{
			2.0f * (static_cast<float>(x) / winSize.x()) - 1.0f,
			2.0f * (static_cast<float>(y) / winSize.y()) - 1.0f,
			m_objectZInClipSpace,
			1.0f
		};
		Engine::math::Vec4f prevPos =
		{
			2.0f * (static_cast<float>(m_mousePrevPos.x()) / winSize.x()) - 1.0f,
			2.0f * (static_cast<float>(m_mousePrevPos.y()) / winSize.y()) - 1.0f,
			m_objectZInClipSpace,
			1.0f
		};

		m_camera.transformFromClipToViewSpace(currentPos);
		m_camera.transformFromClipToViewSpace(prevPos);

		Engine::math::Vec2f posDiff =
		{
			currentPos.x() - prevPos.x(),
			currentPos.y() - prevPos.y()
		};
		
		Engine::math::Vec3f objectMovement{ 0.0f, 0.0f, 0.0f };

		Engine::math::Vec3f camRight = m_camera.right();
		Engine::math::Vec3f camTop = m_camera.top();

		objectMovement += camRight * posDiff[0];
		objectMovement += camTop * posDiff[1];

		m_sceneObjectMover->move(objectMovement);

		//m_mousePrevPos = {x, y};
	}
#endif
}

void Application::onMouseScroll(float zDelta)
{
	m_cameraMovementSpeed *= (1.0f + 0.05f * zDelta);
}

void Application::onRightMouseButtonDown(int x, int y)
{
	Engine::math::Ray ray = m_camera.generateRay(Engine::math::Vec2i(x, y), m_window.getSize());
	
	Engine::MeshSystem::MeshIntersectionQuery query;
	query.mover = &m_sceneObjectMover;
	query.nearest.reset();
	
	Engine::MeshSystem::getInstancePtr()->findIntersection(ray, query);
	
	Engine::math::Vec4f objPos =
	{ 
		query.nearest.position.x(), 
		query.nearest.position.y(), 
		query.nearest.position.z(), 
		1.0f
	};

	m_camera.transformFromWorldToClipSpace(objPos);
	m_objectZInClipSpace = objPos.z();


	m_mousePrevPos = { x, y };
}

void Application::onRightMouseButtonUp(int x, int y)
{
	m_sceneObjectMover.reset();
}

void Application::onFocusLost()
{
	m_rotateCamera = false;
}

void Application::initCamera()
{
	m_camera.setWorldOffset(Engine::math::Vec3f(0.0f, 2.0f, -10.0f));
	m_camera.setPerspective(45.0f, static_cast<float>(m_window.getSize().x()) / m_window.getSize().y(), 0.1f, 100.0f);
	//m_camera.setOrthographic(10, -10, 10, -10, -10, 100);
	m_camera.updateCamera();

	m_prevCamera = m_camera;

	m_cameraMovementSpeed = 10.0f;
	m_cameraMovementSpeedup = 5.0f;
	m_cameraRotationSpeed = 180.0f;

	camRelativeOffset = camRelativeAngles = Engine::math::Vec3f(0.0f, 0.0f, 0.0f);
}

void Application::update()
{
	if (isFlashlightAttachedToCamera)
	{
		m_sceneElementManager.setSpotLightTransform(m_camera.getViewInv());
	}


	auto& dissolutionInstances = Engine::MeshSystem::getInstancePtr()->getDissolutionInstances();
	std::unordered_set<unsigned int> instancesToMove;

	for (int modelIndex = 0; modelIndex < dissolutionInstances.getModels().size(); modelIndex++)
	{
		auto& perModel = dissolutionInstances.getModels()[modelIndex];

		for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
		{
			auto& perMesh = perModel.perMesh[meshIndex];

			for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
			{
				auto& perMaterial = perMesh.perMaterial[materialIndex];

				std::vector<int> instanceToErase;
				for(int instanceIndex = 0; instanceIndex < perMaterial.instances.size(); instanceIndex++)
				{
					auto& instance = perMaterial.instances[instanceIndex];

					if (instance.time >= 1.0f)
					{
						unsigned int objectID = dissolutionInstances.getObjectID(modelIndex, meshIndex, materialIndex, instanceIndex);
						instancesToMove.insert(objectID);
						instanceToErase.push_back(instanceIndex);
						/*bool add = true;
						for (auto& toMove : instancesToMove)
						{
							if (toMove.first->name == perMaterial.material->name)
							{
								add = false;
								break;
							}
						}
						if (add)
						{
							auto litMaterial = std::make_shared< Engine::ShadingGroupsDetails::LitMaterial>();

							litMaterial->name = perMaterial.material->name;

							litMaterial->textureDiffuse = perMaterial.material->textureDiffuse;
							litMaterial->textureNormal = perMaterial.material->textureNormal;
							litMaterial->textureARM = perMaterial.material->textureARM;

							litMaterial->useDefaultRoughness = perMaterial.material->useDefaultRoughness;
							litMaterial->useDefaultMetalness = perMaterial.material->useDefaultMetalness;

							instancesToMove.push_back(std::pair<std::shared_ptr<Engine::ShadingGroupsDetails::LitMaterial>, Engine::TransformSystem::ID>(litMaterial, iter->modelToWorldID));
						}

						iter = perMaterial.instances.erase(iter);*/
					}
					else
					{
						instance.time += 0.1f * m_deltaTime;
					}
				}
			}
		}
		auto* trSystem = Engine::TransformSystem::getInstance();

		for (auto& toMove : instancesToMove)
		{
			using Material = Engine::ShadingGroupsDetails::DissolutionMaterial;
			using Instance = Engine::ShadingGroupsDetails::DissolutionInstance;
			using ShadingGroupT = Engine::ShadingGroup<Material, Instance>;

			Engine::DissolutionInstances::PerModel removed;
			//ShadingGroupT::PerMesh* mesh;
			//ShadingGroupT::PerMaterial* material;
			//Instance* instance;

			dissolutionInstances.removeObjectByID(toMove, removed);
			
			Engine::LitInstances::PerModel litModel;
			litModel.model = removed.model;

			int transformID = trSystem->createMatrix();
			for (int meshIndex = 0; meshIndex < removed.perMesh.size(); meshIndex++)
			{
				auto& perMesh = removed.perMesh[meshIndex];

				Engine::LitInstances::PerMesh perMeshToAdd;
				for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
				{
					auto& perMaterial = perMesh.perMaterial[materialIndex];

					Engine::LitInstances::PerMaterial perMaterialToAdd;
					perMaterialToAdd.material = std::make_shared< Engine::ShadingGroupsDetails::LitMaterial>();
					perMaterialToAdd.material->name = perMaterial.material->name;
					perMaterialToAdd.material->textureDiffuse = perMaterial.material->textureDiffuse;
					perMaterialToAdd.material->textureNormal = perMaterial.material->textureNormal;
					perMaterialToAdd.material->textureARM = perMaterial.material->textureARM;
					perMaterialToAdd.material->useDefaultRoughness = perMaterial.material->useDefaultRoughness;
					perMaterialToAdd.material->useDefaultMetalness = perMaterial.material->useDefaultMetalness;

					auto& instance = perMaterial.instances.front();

					Engine::ShadingGroupsDetails::LitInstance instanceToAdd;
					instanceToAdd.modelToWorldID = transformID;

					auto& transform = trSystem->getMatrix(instanceToAdd.modelToWorldID);
					transform = trSystem->getMatrix(instance.modelToWorldID);

					perMaterialToAdd.instances.push_back(instanceToAdd);
					perMeshToAdd.perMaterial.push_back(perMaterialToAdd);
				}
				litModel.perMesh.push_back(perMeshToAdd);
			}

			Engine::MeshSystem::getInstancePtr()->addLitInstance(litModel);

			//Engine::math::Mat4f transform = Engine::TransformSystem::getInstance()->getMatrix(instance->modelToWorldID);
			//
			//auto litMaterial = std::make_shared< Engine::ShadingGroupsDetails::LitMaterial>();
			//
			//litMaterial->name = material->material->name;
			//
			//litMaterial->textureDiffuse = material->material->textureDiffuse;
			//litMaterial->textureNormal = material->material->textureNormal;
			//litMaterial->textureARM = material->material->textureARM;
			//
			//litMaterial->useDefaultRoughness = material->material->useDefaultRoughness;
			//litMaterial->useDefaultMetalness = material->material->useDefaultMetalness;
			//
			//m_sceneElementManager.addLitModelElement(perModel.model, litMaterial , transform);
			//
			//Engine::MeshSystem::getInstancePtr()->removeObjectByID(toMove);
		}
	}

	std::set<unsigned int> toRemove;
	auto& incinerationInstances = Engine::MeshSystem::getInstancePtr()->getIncinerationInstances();
	for (int modelIndex = 0; modelIndex < incinerationInstances.getModels().size(); modelIndex++)
	{
		auto& perModel = incinerationInstances.getModels()[modelIndex];

		for (int meshIndex = 0; meshIndex < perModel.perMesh.size(); meshIndex++)
		{
			auto& perMesh = perModel.perMesh[meshIndex];

			for (int materialIndex = 0; materialIndex < perMesh.perMaterial.size(); materialIndex++)
			{
				auto& perMaterial = perMesh.perMaterial[materialIndex];

				std::vector<int> instanceToErase;
				for (int instanceIndex = 0; instanceIndex < perMaterial.instances.size(); instanceIndex++)
				{
					auto& instance = perMaterial.instances[instanceIndex];

					instance.spherePrevioursBigRadius = instance.sphereBigRadius;
					instance.sphereBigRadius += 0.5f * m_deltaTime;

					if (instance.sphereBigRadius >= 0.4f) //small radius delay
					{
						instance.sphereSmallRadius += 0.5f * m_deltaTime;
					}

					float modelBBDiagonalLength = perModel.model->getBoundingBox().size().norm();
					if (instance.sphereSmallRadius >= modelBBDiagonalLength)
					{
						unsigned int objectID = incinerationInstances.getObjectID(modelIndex, meshIndex, materialIndex, instanceIndex);
						toRemove.insert(objectID);
					}
				}
			}
		}
	}

	for (auto id : toRemove)
	{
		Engine::IncinerationInstances::PerModel removed;
		incinerationInstances.removeObjectByID(id, removed);
	}
}

void Application::updateTime()
{
	m_timer.newFrameStarted();
	m_deltaTime = m_timer.getDeltaTime();
	m_timeSinceStart += m_deltaTime;

	m_window.setWindowTitle(std::to_string(m_deltaTime) + " s    " + std::to_string(1.0f / m_deltaTime) + " FPS");
}

void Application::processImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Render Configuration");

	auto* renderer = Engine::Renderer::getInstancePtr();
	
	if (ImGui::CollapsingHeader("Lighting"))
	{
		bool useDiffuseReflection = renderer->shouldUseDiffuseReflection();
		if (ImGui::Checkbox("Toggle diffuse reflection", &useDiffuseReflection))
		{
			renderer->setShouldUseDiffuseReflection(useDiffuseReflection);
		}

		bool useSpecularReflection = renderer->shouldUseSpecularReflection();
		if (ImGui::Checkbox("Toggle specular reflection", &useSpecularReflection))
		{
			renderer->setShouldUseSpecularReflection(useSpecularReflection);
		}

		bool useIBL = renderer->shouldUseIBL();
		if (ImGui::Checkbox("Toggle IBL", &useIBL))
		{
			renderer->setShouldUseIBL(useIBL);
		}
	}
	if (ImGui::CollapsingHeader("Material"))
	{
		bool useRoughnessOverwriting = renderer->shouldOverwriteRoughness();
		if (ImGui::Checkbox("Toggle roughness overwriting", &useRoughnessOverwriting))
		{
			renderer->setShouldOverwriteRoughness(useRoughnessOverwriting);
		}

		if (useRoughnessOverwriting)
		{
			float overwrittenRoughness = renderer->getOverwrittenRoughnessValue();
			if (ImGui::SliderFloat("Overwriten roughness value", &overwrittenRoughness, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
			{
				renderer->setOverwrittenRoughnessValue(overwrittenRoughness);
			}
		}
	}
	if (ImGui::CollapsingHeader("FXAA"))
	{
		float fxaaQualitySubpix = renderer->getFXAAQualitySubpix();
		if (ImGui::SliderFloat("FXAA Quality Subpix", &fxaaQualitySubpix, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			renderer->setFXAAQualitySubpix(fxaaQualitySubpix);
		}

		float fxaaQualityEdgeThreshold = renderer->getFXAAQualityEdgeThreshhold();
		if (ImGui::SliderFloat("FXAA Quality Edge Threshold", &fxaaQualityEdgeThreshold, 0.063f, 0.333f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			renderer->setFXAAQualityEdgeThreshhold(fxaaQualityEdgeThreshold);
		}

		float fxaaQualityEdgeThresholdMin = renderer->getFXAAQualityEdgeThreshholdMin();
		if (ImGui::SliderFloat("FXAA Quality Edge Threshold Min", &fxaaQualityEdgeThresholdMin, 0.0312f, 0.0833f, "%.4f", ImGuiSliderFlags_AlwaysClamp))
		{
			renderer->setFXAAQualityEdgeThreshholdMin(fxaaQualityEdgeThresholdMin);
		}
	}
	if (ImGui::CollapsingHeader("Fog"))
	{
		bool uniformFogEnabled = renderer->isUniformFogEnabled();
		if (ImGui::Checkbox("Enable uniform fog", &uniformFogEnabled))
		{
			if (uniformFogEnabled)
			{
				renderer->enableUniformFog();
			}
		}

		bool volumetricFogEnabled = renderer->isVolumetricFogEnabled();
		if (ImGui::Checkbox("Enable volumetric fog", &volumetricFogEnabled))
		{
			if (volumetricFogEnabled)
			{
				renderer->enableVolumetricFog();
			}
		}

		if(!uniformFogEnabled && !volumetricFogEnabled)
		{
			renderer->disableFog();
		}

		float fogAb = renderer->getFogDensity();
		if (ImGui::SliderFloat("Fog density", &fogAb, 0.0f, 1.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
		{
			renderer->setFogDensity(fogAb);
		}

		float fogP = renderer->getFogPhaseFunctionParameter();
		if (ImGui::SliderFloat("Fog phase function parameter", &fogP, -1.0f, 1.0f, "%.4f", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic))
		{
			renderer->setFogPhaseFunctionParameter(fogP);
		}
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
