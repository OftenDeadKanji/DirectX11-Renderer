#pragma once
#include "utils/FPSTimer.h"
#include "Listener/eventDispatcher.h"

#include "render/scene/scene.h"
#include "window/window.h"
#include "render/camera/camera.h"

#include "objectMover/IObjectMover.h"
#include <memory>
#include <unordered_map>

#include "sceneElementManager.h"

class Application
	: public IWindowListener
{
public:
	Application(HWND windowHandle);
	~Application();

	void run();

	void processInputs();

	static EventDispatcher eventDispatcher;

	void onWindowClose() override;
	void onWindowResize(int width, int height) override;
	void onKeyDown(wchar_t key) override;
	void onKeyUp(wchar_t key) override;
	void onLeftMouseButtonDown(int x, int y) override;
	void onLeftMouseButtonUp(int x, int y) override;
	void onMouseMove(int x, int y) override;

	void onMouseScroll(float zDelta) override;

	void onRightMouseButtonDown(int x, int y) override;
	void onRightMouseButtonUp(int x, int y) override;

	void onFocusLost() override;
private:
	void initCamera();

	void update();
	void updateTime();

	void processImGui();

	void createLights();
	void createLitObjects();
	void createTextureOnlyObjects();
	void createHologramObjects();
	
	void createParticles();

	void spawnDissolutionObject(Engine::math::Vec3f position);

	bool m_mainLoopCondition = true;

	Engine::FPSTimer m_timer;
	float m_deltaTime = 0.0f;
	float m_timeSinceStart = 0.0f;

	Engine::Window m_window;

	std::unordered_map<wchar_t, bool> m_keyStates;

	// Camera, its movement and rotation
	Engine::Camera m_camera;
	Engine::Camera m_prevCamera;

	Engine::math::Vec3f camRelativeOffset;
	Engine::math::Vec3f camRelativeAngles;

	int m_numKeysDown = 0;
	float m_cameraMovementSpeed;
	float m_cameraMovementSpeedup;

	bool m_rotateCamera = false;
	Engine::math::Vec2i m_cameraRotationMouseStartPos;
	Engine::math::Vec2i m_cameraRotationMouseCurrentPos;
	float m_cameraRotationSpeed; // deg/sec when mousePosDiff is half the window width

	bool isFlashlightAttachedToCamera = true;

	//scene objects movement
	std::unique_ptr<Engine::IObjectMover> m_sceneObjectMover;
	Engine::math::Vec2i m_mousePos;
	Engine::math::Vec2i m_mousePrevPos;
	float m_objectZInClipSpace;

	Engine::math::Vec3f mousePositionInWS;

	bool isNormalVisualizationOn = false;

	float m_ev100 = 1.0f;
	float m_ev100ChangeValue = 1.0f;

	SceneElementManager m_sceneElementManager;
};

