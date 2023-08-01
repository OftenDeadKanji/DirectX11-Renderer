#include "eventDispatcher.h"

void EventDispatcher::addWindowListener(IWindowListener* listener)
{
	m_windowListeners.push_back(listener);
}

void EventDispatcher::removeWindowListener(IWindowListener* listener)
{
	for (auto iter = m_windowListeners.begin(); iter != m_windowListeners.end(); iter++)
	{
		if ((*iter) == listener)
		{
			m_windowListeners.erase(iter);
			return;
		}
	}
}

void EventDispatcher::onWindowClose()
{
	for (auto listener : m_windowListeners)
	{
		listener->onWindowClose();
	}
}

void EventDispatcher::onWindowResize(int width, int height)
{
	for (auto listener : m_windowListeners)
	{
		listener->onWindowResize(width, height);
	}
}

void EventDispatcher::onKeyDown(wchar_t key)
{
	for (auto listener : m_windowListeners)
	{
		listener->onKeyDown(key);
	}
}

void EventDispatcher::onKeyUp(wchar_t key)
{
	for (auto listener : m_windowListeners)
	{
		listener->onKeyUp(key);
	}
}

void EventDispatcher::onLeftMouseButtonDown(int x, int y)
{
	for (auto listener : m_windowListeners)
	{
		listener->onLeftMouseButtonDown(x, y);
	}
}

void EventDispatcher::onLeftMouseButtonUp(int x, int y)
{
	for (auto listener : m_windowListeners)
	{
		listener->onLeftMouseButtonUp(x, y);
	}
}

void EventDispatcher::onRightMouseButtonDown(int x, int y)
{
	for (auto listener : m_windowListeners)
	{
		listener->onRightMouseButtonDown(x, y);
	}
}

void EventDispatcher::onRightMouseButtonUp(int x, int y)
{
	for (auto listener : m_windowListeners)
	{
		listener->onRightMouseButtonUp(x, y);
	}
}

void EventDispatcher::onMouseMove(int x, int y)
{
	for (auto listener : m_windowListeners)
	{
		listener->onMouseMove(x, y);
	}
}

void EventDispatcher::onMouseScroll(float zDelta)
{
	for (auto listener : m_windowListeners)
	{
		listener->onMouseScroll(zDelta);
	}
}

void EventDispatcher::onFocusLost()
{
	for (auto listener : m_windowListeners)
	{
		listener->onFocusLost();
	}
}
