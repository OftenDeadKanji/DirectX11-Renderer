#include <iostream>
#include "application.h"
#include "dependencies/Windows/win.h"
#include <Windowsx.h>
#include "engine/engine.h"
#include "utils/debug/debugOutput.h"

#include "imgui/imgui_impl_win32.h"

HWND createWinAPIWindow(int width, int height, HINSTANCE appHandle, int windowShowParams);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

Engine::math::Vec3f randomHemisphere(float& NdotV, float i, float N)
{
	const float GOLDEN_RATIO = (1.0 + sqrt(5.0)) / 2.0;
	float theta = 2.0 * Engine::math::PI * i / GOLDEN_RATIO;
	float phiCos = NdotV = 1.0 - (i + 0.5) / N;
	float phiSin = sqrt(1.0 - phiCos * phiCos);
	float thetaCos = std::cos(theta), thetaSin = std::sin(theta);
	return Engine::math::Vec3f(thetaCos * phiSin, thetaSin * phiSin, phiCos);
}

void testRandomHemisphereFibonacci()
{
	int N = 10000;
	float sum = 0;
	for (int i = 0; i < N; i++)
	{
		float dot = 0.0f;
		randomHemisphere(dot, i, N);

		sum += dot;
	}

	float result = ((2.0f * Engine::math::PI) / N) * sum;

	_DEBUG_OUTPUT(result); // in Output window
}


int WINAPI WinMain(HINSTANCE appHandle, HINSTANCE, LPSTR cmdLine, int windowShowParams)
{
	testRandomHemisphereFibonacci();

	Engine::Engine::init();

	auto windowHandle = createWinAPIWindow(800, 800, appHandle, windowShowParams);

	{
		Application app(windowHandle);
		app.run();
	}
	

	Engine::Engine::deinit();
}

HWND createWinAPIWindow(int width, int height, HINSTANCE appHandle, int windowShowParams)
{
	WNDCLASSEX wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = appHandle;
	wcex.hIcon = LoadIcon(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass1";
	wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);

	RegisterClassEx(&wcex);

	RECT rect = { 0, 0, width, height };
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, false, NULL);

	auto adjustedWidth = rect.right - rect.left;
	auto adjustedHeight = rect.bottom - rect.top;

	HWND hWnd = CreateWindowEx(
		NULL,
		L"WindowClass1",
		L"Homework_1",
		WS_OVERLAPPEDWINDOW,
		300, 300,
		adjustedWidth, adjustedHeight,
		NULL,
		NULL,
		appHandle,
		NULL
	);

	ShowWindow(hWnd, windowShowParams);

	return hWnd;
}

WPARAM processKey(WPARAM wparam, LPARAM lParam)
{
	WPARAM key = wparam;
	UINT scancode = (lParam & 0x00ff0000) >> 16;
	int extended = (lParam & 0x01000000) != 0;

	switch (key) {
	case VK_SHIFT:
		key = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
		break;
	case VK_CONTROL:
		key = extended ? VK_RCONTROL : VK_LCONTROL;
		break;
	case VK_MENU:
		key = extended ? VK_RMENU : VK_LMENU;
		break;
	}

	return key;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	RECT rect;
	GetClientRect(hWnd, &rect);

	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	auto key = processKey(wParam, lParam);
	switch (message)
	{
	case WM_DESTROY:
		Application::eventDispatcher.onWindowClose();
		break;
	case WM_SIZE:
		Application::eventDispatcher.onWindowResize(width, height);
		break;
	case WM_KEYDOWN:
		if ((HIWORD(lParam) & KF_REPEAT) != KF_REPEAT) // one-time action (without repeating when key is held down)
		{
			Application::eventDispatcher.onKeyDown((wchar_t)key);
		}
		break;
	case WM_KEYUP:
		Application::eventDispatcher.onKeyUp((wchar_t)key);
		break;
	case WM_LBUTTONDOWN:
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			Application::eventDispatcher.onLeftMouseButtonDown(GET_X_LPARAM(lParam), height - GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONUP:
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			Application::eventDispatcher.onLeftMouseButtonUp(GET_X_LPARAM(lParam), height - GET_Y_LPARAM(lParam));
		}
		break;
	case WM_RBUTTONDOWN:
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			Application::eventDispatcher.onRightMouseButtonDown(GET_X_LPARAM(lParam), height - GET_Y_LPARAM(lParam));
		}
		break;
	case WM_RBUTTONUP:
		if (!ImGui::GetIO().WantCaptureMouse)
		{
			Application::eventDispatcher.onRightMouseButtonUp(GET_X_LPARAM(lParam), height - GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		Application::eventDispatcher.onMouseMove(GET_X_LPARAM(lParam), height - GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSEWHEEL:
		Application::eventDispatcher.onMouseScroll(static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA);
		break;
	case WM_KILLFOCUS:
		Application::eventDispatcher.onFocusLost();
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}