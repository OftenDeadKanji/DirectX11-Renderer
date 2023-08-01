#pragma once
#include "IWindowListener.h"
#include <vector>

class EventDispatcher
{
public:
	void addWindowListener(IWindowListener*);
	void removeWindowListener(IWindowListener*);

	void onWindowClose();
	void onWindowResize(int width, int height);
	void onKeyDown(wchar_t key);
	void onKeyUp(wchar_t key);
	void onLeftMouseButtonDown(int x, int y);
	void onLeftMouseButtonUp(int x, int y);

	void onRightMouseButtonDown(int x, int y);
	void onRightMouseButtonUp(int x, int y);

	void onMouseMove(int x, int y);

	void onMouseScroll(float zDelta);

	void onFocusLost();
private:
	std::vector<IWindowListener*> m_windowListeners;
};