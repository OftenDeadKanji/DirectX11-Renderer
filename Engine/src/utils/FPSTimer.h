#pragma once
#include <chrono>

namespace Engine
{
	class FPSTimer
	{
	public:
		FPSTimer(unsigned int desiredFramerate);

		void setDesiredFramerate(unsigned int desiredFramerate);
		bool frameElapsed() const;
		void newFrameStarted();
		float getDeltaTime() const;

	private:
		using std_clock = std::chrono::high_resolution_clock;

		std_clock::time_point m_lastFrame;
		unsigned int m_desiredFramerate;
		float m_deltaTime = 0.0f;
	};
}
