#if EE_PLATFORM_LINUX
#pragma once

#include "Base/Application/ApplicationGlobalState.h"
#include "Base/Esoterica.h"
#include "Base/Types/String.h"
#include "Base/Math/Math.h"
#include "Base/Types/BitFlags.h"

// Forward declare SDL types to avoid hard dependency in header
struct SDL_Window;
typedef void* SDL_GLContext;

//-------------------------------------------------------------------------

namespace EE
{
    namespace Math { class ScreenSpaceRectangle; }

    //-------------------------------------------------------------------------
    // Linux Application — SDL3/SDL2 or headless fallback
    // Mirrors Win32Application interface for cross-platform code sharing
    //-------------------------------------------------------------------------

    class EE_BASE_API LinuxApplication
    {
    protected:

        enum class InitOptions
        {
            StartMinimized,
            Borderless
        };

    public:

        LinuxApplication( char const* applicationName, int32_t iconResourceID = -1, int32_t splashScreenResourceID = -1, TBitFlags<InitOptions> options = TBitFlags<InitOptions>() );
        virtual ~LinuxApplication();

        int32_t Run( int32_t argc, char** argv );

        inline bool WasInitialized() const { return m_initialized; }

        // Called whenever we receive an application exit request. Return true to allow the exit
        virtual bool OnUserExitRequest() { return true; }

        // Get native window handle (SDL_Window* on Linux, HWND on Windows)
        inline void* GetWindowHandle() const { return m_pWindow; }
        inline SDL_Window* GetSDLWindow() const { return m_pWindow; }

    protected:

        virtual bool FatalError( String const& error ) const;

        //-------------------------------------------------------------------------

        virtual void OnFirstShowMainWindow() {}
        virtual void ProcessWindowDestructionMessage();
        virtual void ResizeMainWindow( Int2 const& newWindowSize ) = 0;

        // Input handling — Linux receives SDL events instead of Win32 messages
        // For compatibility, we provide both SDL event hook and legacy ProcessInputMessage stub
        virtual void ProcessSDLEvent( void* pSDLEvent ) {}
        virtual void ProcessInputMessage( uint32_t message, uintptr_t wParam, intptr_t lParam ) {}

        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const {}

        //-------------------------------------------------------------------------

        virtual void WriteWindowSettings();
        virtual void ReadWindowSettings();

        virtual bool Initialize( int32_t argc, char** argv ) = 0;
        virtual bool Shutdown() = 0;
        virtual bool ApplicationLoop() = 0;

        //-------------------------------------------------------------------------

        void RequestApplicationExit() { m_applicationRequestedExit = true; }

    private:

        bool TryCreateMainWindow();
        void ShowMainWindow();
        bool TryCreateSplashScreen();
        void DestroySplashScreen();
        void HandleSDLEvents();

    protected:

        String const                    m_applicationName;
        String const                    m_applicationNameNoWhitespace;
        int32_t                         m_applicationIconResourceID = -1;

        SDL_Window*                     m_pWindow = nullptr;
        Int2                            m_windowSize = Int2( 1280, 720 );
        Int2                            m_windowPosition = Int2( 100, 100 );

        int32_t                         m_splashScreenBitmapResourceID = -1;
        void*                           m_pSplashWindow = nullptr;

    private:

        bool                            m_wasMaximized = false;
        bool                            m_startMinimized = false;
        bool                            m_initialized = false;
        bool                            m_applicationRequestedExit = false;
        bool                            m_isBorderLess = false;
    };

    // Alias for cross-platform code
    using PlatformApplication = LinuxApplication;
}

// If SDL is not available, provide minimal stubs so code compiles headless
#if !defined(EE_WITH_SDL3) && !defined(EE_WITH_SDL2)
    // Headless fallback: Application will run without window (e.g., ResourceCompiler/Server)
#endif

#else
    // On Windows, alias to Win32Application for shared code
    #include "Application_Win32.h"
    namespace EE { using PlatformApplication = Win32Application; }
#endif
