#include "FileDialogs.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>


namespace FileDialogs 
{
	std::wstring OpenFile(const wchar_t* filters)
	{
		OPENFILENAME ofn;
		wchar_t szFile[260] = { 0 }; // buffer for file name

		// Initialize OPENFILENAME
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = 0;
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filters;
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileName(&ofn) == TRUE)
		{
			return ofn.lpstrFile;
		}

		return std::wstring();
	}
}