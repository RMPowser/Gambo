#pragma once
#include <string>

namespace FileDialogs
{
	// returns empty string if canceled 
	std::wstring OpenFile(const wchar_t* filter);
}