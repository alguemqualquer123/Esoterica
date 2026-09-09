#if EE_PLATFORM_LINUX
#include "ImguiPlatform_Linux.h"

#if EE_DEVELOPMENT_TOOLS

#if defined(EE_WITH_SDL3)
    #include <SDL3/SDL.h>
#elif defined(EE_WITH_SDL2)
    #include <SDL2/SDL.h>
#endif

//-------------------------------------------------------------------------

namespace EE::ImGuiX::Platform
{
    bool ProcessSDLEvent( void* pSDLEvent )
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        // Forward to ImGui SDL backend if available
        // ImGui_ImplSDL3_ProcessEvent or ImGui_ImplSDL2_ProcessEvent will be called by ImguiSystem
        // This stub allows Application_Linux to route events correctly
        (void) pSDLEvent;
        return false;
        #else
        (void) pSDLEvent;
        return false;
        #endif
    }
}
#endif
#endif
