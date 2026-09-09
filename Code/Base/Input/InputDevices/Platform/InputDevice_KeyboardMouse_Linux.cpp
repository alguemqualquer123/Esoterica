#if EE_PLATFORM_LINUX
#include "Base/Input/InputDevices/InputDevice_KeyboardMouse.h"
#include "Base/Types/HashMap.h"

#if defined(EE_WITH_SDL3)
    #include <SDL3/SDL.h>
#elif defined(EE_WITH_SDL2)
    #include <SDL2/SDL.h>
#endif

//-------------------------------------------------------------------------

namespace EE::Input
{
    // Linux keyboard/mouse via SDL scancodes
    // Full mapping mirrors WindowsKeyMap but uses SDL_Scancode/SDL_Keycode

    void KeyboardMouseInputDevice::PlatformInitialize()
    {
        // SDL handles keyboard state internally; nothing to init here
    }

    void KeyboardMouseInputDevice::PlatformShutdown()
    {
    }

    bool KeyboardMouseInputDevice::PlatformPollEvents()
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        // Events are polled in LinuxApplication::HandleSDLEvents and forwarded via ProcessSDLEvent
        // This device reads current SDL keyboard/mouse state
        #endif
        return true;
    }

    // SDL scancode to EE InputID conversion would go here
    // For now, stub — actual mapping can be ported from WindowsKeyMap using SDL key tables
}
#endif
