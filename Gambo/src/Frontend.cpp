#include "Frontend.h"

#include "imgui.h"

#ifdef WIN32_DX12
	#include "imgui_impl_win32.h"
	#include "imgui_impl_dx12.h"
	#include <d3d12.h>
	#include <dxgi1_4.h>

	#ifdef _DEBUG
	#define DX12_ENABLE_DEBUG_LAYER
	#endif

	#ifdef DX12_ENABLE_DEBUG_LAYER
	#include <dxgidebug.h>
	#pragma comment(lib, "dxguid.lib")
	#endif
#endif

Frontend::Frontend()
{
	// create window
	WNDCLASSEXW wc;
}

Frontend::~Frontend()
{
}