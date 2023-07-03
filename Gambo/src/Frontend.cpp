#include "Frontend.h"

#include "imgui.h"

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

Frontend::Frontend()
{
}

Frontend::~Frontend()
{
}