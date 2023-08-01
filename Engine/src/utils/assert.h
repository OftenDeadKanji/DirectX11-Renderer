#pragma once
#include <cstdlib>

#define BREAK __debugbreak();

#define ALWAYS_ASSERT(expression, ...) \
	if(!(expression)) \
	{ \
		BREAK; \
		std::abort(); \
	} \
	else {}

#ifdef NDEBUG
#define DEV_ASSERT(...)
#else
#define DEV_ASSERT(expression, ...) ALWAYS_ASSERT(expression, __VA_ARGS__);
#endif