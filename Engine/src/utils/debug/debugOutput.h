#pragma once
#include "../../dependencies/Windows/win.h"
#include <iostream>
#include <sstream>

#define _DEBUG_OUTPUT( log )					\
{												\
	std::wostringstream os_;					\
	os_ << "\nFile: " << __FILE__;				\
	os_ << "\nLine: " << __LINE__;				\
	os_ << "\nLog: " << log << "\n";			\
	OutputDebugStringW( os_.str().c_str() );	\
}