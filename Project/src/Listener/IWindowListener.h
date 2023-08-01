#pragma once

class IWindowListener
{
public:
	virtual void onWindowClose() = 0;
	virtual void onWindowResize(int width, int height) = 0;

	virtual void onMouseMove(int x, int y) = 0;
	
	virtual void onMouseScroll(float zDelta) = 0;

	virtual void onLeftMouseButtonDown(int x, int y) = 0;
	virtual void onLeftMouseButtonUp(int x, int y) = 0;
	
	virtual void onRightMouseButtonDown(int x, int y) = 0;
	virtual void onRightMouseButtonUp(int x, int y) = 0;

	virtual void onKeyDown(wchar_t key) = 0;
	virtual void onKeyUp(wchar_t key) = 0;

	virtual void onFocusLost() = 0;
};