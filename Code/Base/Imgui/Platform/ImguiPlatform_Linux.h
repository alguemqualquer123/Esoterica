#if EE_PLATFORM_LINUX
#pragma once

#include "Base/_Module/API.h"
#include "Base/Esoterica.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX::Platform
{
    // SDL event processor — returns true if event was handled by ImGui
    EE_BASE_API bool ProcessSDLEvent( void* pSDLEvent );

    // Legacy Win32 message processor stub for cross-platform code — always returns 0 on Linux
    EE_BASE_API inline intptr_t WindowMessageProcessor( void* /*hWnd*/, uint32_t /*message*/, uintptr_t /*wParam*/, intptr_t /*lParam*/ ) { return 0; }
}
#endif
#endif
