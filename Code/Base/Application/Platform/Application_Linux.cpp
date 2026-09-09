#if EE_PLATFORM_LINUX
#include "Application_Linux.h"
#include "Base/Settings/IniFile.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Logging/SystemLog.h"
#include "Base/Math/Rectangle.h"
#include "Base/Platform/Platform.h"

#include <cstdio>
#include <cstring>

#if defined(EE_WITH_SDL3)
    #include <SDL3/SDL.h>
#elif defined(EE_WITH_SDL2)
    #include <SDL2/SDL.h>
#endif

//-------------------------------------------------------------------------

namespace EE
{
    LinuxApplication::LinuxApplication( char const* applicationName, int32_t iconResourceID, int32_t splashScreenResourceID, TBitFlags<InitOptions> options )
        : m_applicationName( applicationName )
        , m_applicationNameNoWhitespace( StringUtils::StripAllWhitespace( String( applicationName ) ) )
        , m_applicationIconResourceID( iconResourceID )
        , m_splashScreenBitmapResourceID( splashScreenResourceID )
        , m_startMinimized( options.IsFlagSet( InitOptions::StartMinimized ) )
        , m_isBorderLess( options.IsFlagSet( InitOptions::Borderless ) )
    {}

    LinuxApplication::~LinuxApplication()
    {
        Platform::ClearMainWindowHandle();
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        if ( m_pWindow != nullptr )
        {
            SDL_DestroyWindow( m_pWindow );
            m_pWindow = nullptr;
        }
        SDL_Quit();
        #endif
    }

    bool LinuxApplication::FatalError( String const& error ) const
    {
        fprintf( stderr, "Fatal Error: %s\n", error.c_str() );
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Fatal Error Occurred!", error.c_str(), m_pWindow );
        #endif
        return false;
    }

    //-------------------------------------------------------------------------

    bool LinuxApplication::TryCreateMainWindow()
    {
        EE_ASSERT( m_windowSize.m_x > 0 && m_windowSize.m_y > 0 );

        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)

        if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD ) != 0 )
        {
            EE_LOG_ERROR( LogCategory::General, "Application/Linux", "SDL_Init failed: %s", SDL_GetError() );
            // Fallback to headless if SDL unavailable — allow console tools to run
            return false;
        }

        Uint32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
        #if defined(EE_WITH_SDL3)
        windowFlags |= m_isBorderLess ? SDL_WINDOW_BORDERLESS : 0;
        if ( m_startMinimized ) windowFlags |= SDL_WINDOW_MINIMIZED;
        if ( m_wasMaximized ) windowFlags |= SDL_WINDOW_MAXIMIZED;
        #else
        windowFlags |= m_isBorderLess ? SDL_WINDOW_BORDERLESS : 0;
        if ( m_startMinimized ) windowFlags |= SDL_WINDOW_MINIMIZED;
        if ( m_wasMaximized ) windowFlags |= SDL_WINDOW_MAXIMIZED;
        #endif

        m_pWindow = SDL_CreateWindow(
            m_applicationName.c_str(),
            m_windowPosition.m_x, m_windowPosition.m_y,
            m_windowSize.m_x, m_windowSize.m_y,
            windowFlags
        );

        if ( m_pWindow == nullptr )
        {
            EE_LOG_ERROR( LogCategory::General, "Application/Linux", "SDL_CreateWindow failed: %s", SDL_GetError() );
            return false;
        }

        Platform::SetMainWindowHandle( m_pWindow );
        return true;

        #else
        // Headless fallback — no windowing available (e.g., ResourceCompiler on server)
        EE_LOG_WARNING( LogCategory::General, "Application/Linux", "SDL not available — running headless (no window)" );
        m_pWindow = nullptr;
        return true;
        #endif
    }

    void LinuxApplication::ShowMainWindow()
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        if ( m_pWindow != nullptr )
        {
            SDL_ShowWindow( m_pWindow );
            #if defined(EE_WITH_SDL3)
            SDL_RaiseWindow( m_pWindow );
            #endif
        }
        #endif
    }

    bool LinuxApplication::TryCreateSplashScreen()
    {
        // Splash screen not implemented on Linux — return true to not block startup
        return true;
    }

    void LinuxApplication::DestroySplashScreen()
    {
    }

    void LinuxApplication::HandleSDLEvents()
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        SDL_Event event;
        while ( SDL_PollEvent( &event ) )
        {
            // Allow derived class to handle raw SDL events first
            ProcessSDLEvent( &event );

            switch ( event.type )
            {
                case SDL_EVENT_WINDOW_RESIZED:
                case SDL_WINDOWEVENT_RESIZED:
                {
                    Int2 newSize = m_windowSize;
                    #if defined(EE_WITH_SDL3)
                    SDL_GetWindowSize( m_pWindow, &newSize.m_x, &newSize.m_y );
                    #else
                    SDL_GetWindowSize( m_pWindow, &newSize.m_x, &newSize.m_y );
                    #endif
                    if ( newSize.m_x > 0 && newSize.m_y > 0 )
                    {
                        m_windowSize = newSize;
                        if ( WasInitialized() )
                        {
                            ResizeMainWindow( newSize );
                        }
                    }
                }
                break;

                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                case SDL_WINDOWEVENT_CLOSE:
                {
                    if ( OnUserExitRequest() )
                    {
                        RequestApplicationExit();
                    }
                }
                break;

                case SDL_EVENT_KEY_DOWN:
                case SDL_EVENT_KEY_UP:
                case SDL_EVENT_MOUSE_MOTION:
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                case SDL_EVENT_MOUSE_WHEEL:
                case SDL_EVENT_GAMEPAD_ADDED:
                case SDL_EVENT_GAMEPAD_REMOVED:
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                case SDL_EVENT_GAMEPAD_BUTTON_UP:
                case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                {
                    if ( WasInitialized() )
                    {
                        // Forward as generic input — derived class can override ProcessSDLEvent for detailed handling
                    }
                }
                break;

                default: break;
            }
        }
        #endif
    }

    void LinuxApplication::ProcessWindowDestructionMessage()
    {
        WriteWindowSettings();
    }

    //-------------------------------------------------------------------------

    void LinuxApplication::ReadWindowSettings()
    {
        FileSystem::Path const layoutIniFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + ".layout.ini";
        IniFile layoutIni;
        if ( !layoutIni.Load( layoutIniFilePath ) )
        {
            return;
        }

        m_windowPosition.m_x = (int32_t) layoutIni.GetInt( "WindowSettings", "Left", m_windowPosition.m_x );
        m_windowPosition.m_y = (int32_t) layoutIni.GetInt( "WindowSettings", "Top", m_windowPosition.m_y );
        int32_t w = (int32_t) layoutIni.GetInt( "WindowSettings", "Width", m_windowSize.m_x );
        int32_t h = (int32_t) layoutIni.GetInt( "WindowSettings", "Height", m_windowSize.m_y );
        m_windowSize = Int2( w, h );
        m_wasMaximized = layoutIni.GetBool( "WindowSettings", "WasMaximized", m_wasMaximized );
    }

    void LinuxApplication::WriteWindowSettings()
    {
        #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
        if ( m_pWindow == nullptr ) return;

        int32_t x = m_windowPosition.m_x, y = m_windowPosition.m_y;
        int32_t w = m_windowSize.m_x, h = m_windowSize.m_y;
        SDL_GetWindowPosition( m_pWindow, &x, &y );
        SDL_GetWindowSize( m_pWindow, &w, &h );
        bool isMaximized = ( SDL_GetWindowFlags( m_pWindow ) & SDL_WINDOW_MAXIMIZED ) != 0;

        IniFile layoutIni;
        layoutIni.SetInt( "WindowSettings", "Left", x );
        layoutIni.SetInt( "WindowSettings", "Top", y );
        layoutIni.SetInt( "WindowSettings", "Width", w );
        layoutIni.SetInt( "WindowSettings", "Height", h );
        layoutIni.SetBool( "WindowSettings", "WasMaximized", isMaximized );

        FileSystem::Path const layoutIniFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + ".layout.ini";
        layoutIni.Save( layoutIniFilePath );
        #endif
    }

    //-------------------------------------------------------------------------

    int32_t LinuxApplication::Run( int32_t argc, char** argv )
    {
        ReadWindowSettings();

        if ( !TryCreateMainWindow() )
        {
            // For windowed apps, this is fatal; for headless tools, continue
            #if defined(EE_WITH_SDL3) || defined(EE_WITH_SDL2)
            return FatalError( "Application failed to create window!" ) ? 0 : 1;
            #endif
        }

        if ( !TryCreateSplashScreen() )
        {
            return FatalError( "Application failed to create splash screen!" ) ? 0 : 1;
        }

        if ( !Initialize( argc, argv ) )
        {
            Shutdown();
            return FatalError( "Application failed to initialize correctly!" ) ? 0 : 1;
        }
        else
        {
            m_initialized = true;
        }

        ShowMainWindow();
        DestroySplashScreen();
        OnFirstShowMainWindow();

        int32_t exitCode = 0;
        bool shouldExit = false;
        while ( !shouldExit )
        {
            HandleSDLEvents();

            if ( m_applicationRequestedExit )
            {
                shouldExit = true;
            }
            else
            {
                shouldExit = !ApplicationLoop();
            }

            // Prevent busy loop when headless
            #if !defined(EE_WITH_SDL3) && !defined(EE_WITH_SDL2)
            if ( m_pWindow == nullptr )
            {
                // Headless tools run ApplicationLoop once per frame — no SDL events to poll
                // Small sleep to avoid 100% CPU if ApplicationLoop returns immediately
                // Most tools will exit after one iteration
            }
            #endif
        }

        bool const shutdownResult = Shutdown();
        m_initialized = false;

        FileSystem::Path const logFilePath = FileSystem::GetCurrentProcessPath() + m_applicationNameNoWhitespace + "Log.txt";
        SystemLog::SaveToFile( logFilePath );

        if ( !shutdownResult )
        {
            return FatalError( "Application failed to shutdown correctly!" ) ? 0 : 1;
        }

        return exitCode;
    }
}
#endif
