#if EE_PLATFORM_LINUX
#include "Base/Input/InputDevices/InputDevice_XBoxController.h"

#if defined(EE_WITH_SDL3)
    #include <SDL3/SDL.h>
#elif defined(EE_WITH_SDL2)
    #include <SDL2/SDL.h>
#endif

//-------------------------------------------------------------------------

namespace EE::Input
{
    void XBoxControllerInputDevice::Initialize()
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        // SDL_Gamepad handles controller detection
        // Enumerate already-connected gamepads
        int count = 0;
        #if defined(EE_WITH_SDL3)
        SDL_JoystickID* ids = SDL_GetGamepads( &count );
        m_isConnected = ( count > m_hardwareControllerIdx );
        if ( ids != nullptr ) SDL_free( ids );
        #else
        m_isConnected = ( SDL_NumJoysticks() > m_hardwareControllerIdx );
        #endif
        #else
        m_isConnected = false;
        #endif
    }

    void XBoxControllerInputDevice::Shutdown()
    {
        m_isConnected = false;
    }

    void XBoxControllerInputDevice::Update( Seconds deltaTime )
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        // SDL gamepad state is updated via SDL_PollEvent in Application loop
        // Read button/axis state from SDL_Gamepad if connected
        (void) deltaTime;
        // TODO: map SDL_GamepadButton/SDL_GamepadAxis to EE InputIDs
        // For now, keep connected check
        #else
        (void) deltaTime;
        #endif
    }
}
#endif
