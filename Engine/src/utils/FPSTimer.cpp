#include "FPSTimer.h"

namespace Engine
{
	FPSTimer::FPSTimer(unsigned int desiredFramerate)
		: m_desiredFramerate(desiredFramerate), m_lastFrame(std_clock::now())
	{
	}

	void FPSTimer::setDesiredFramerate(unsigned int desiredFramerate)
	{
		m_desiredFramerate = desiredFramerate;
	}

	bool FPSTimer::frameElapsed() const
	{
		auto now = std_clock::now();

		auto seconds = std::chrono::duration<float>(now - m_lastFrame).count();
		float frameInterval = 1.0f / m_desiredFramerate;


		return seconds >= frameInterval;
	}

	void FPSTimer::newFrameStarted()
	{
		auto now = std_clock::now();
		m_deltaTime = std::chrono::duration<float>(now - m_lastFrame).count();

		m_lastFrame = now;
	}

	float FPSTimer::getDeltaTime() const
	{
		return m_deltaTime;
	}
}