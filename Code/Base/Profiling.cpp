#include "Profiling.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Esoterica.h"

#if EE_ENABLE_SUPERLUMINAL
#include "Superluminal/PerformanceAPI.h"
#endif

#if defined(_WIN32) || EE_PLATFORM_WINDOWS
#include "Base/Platform/PlatformUtils_Win32.h"
#endif
#if defined(__linux__) || EE_PLATFORM_LINUX
#include "Base/Platform/PlatformUtils_Linux.h"
#endif

//-------------------------------------------------------------------------

namespace EE::Profiling
{
    void StartFrame()
    {
        #if EE_ENABLE_SUPERLUMINAL
        PerformanceAPI::BeginEvent( "Frame" );
        #endif

        #if EE_DEVELOPMENT_TOOLS
        OPTICK_FRAME( "EE Main" );
        #endif
    }

    void EndFrame()
    {
        #if EE_ENABLE_SUPERLUMINAL
        PerformanceAPI::EndEvent();
        #endif
    }

    void OpenProfiler()
    {
        #if defined(_WIN32) || EE_PLATFORM_WINDOWS
        FileSystem::Path const profilerPath = FileSystem::Path( Platform::Win32::GetCurrentModulePath() ) + "..\\..\\..\\..\\External\\Optick\\Optick.exe";
        Platform::Win32::StartProcess( profilerPath );
        #endif
        #if defined(__linux__) || EE_PLATFORM_LINUX
        FileSystem::Path const profilerPathLinux = FileSystem::Path( Platform::Linux::GetCurrentModulePath() ).GetParentDirectory().GetParentDirectory().GetParentDirectory().GetParentDirectory() + "External/Optick/Optick";
        if ( FileSystem::Exists( profilerPathLinux ) )
        {
            Platform::Linux::StartProcess( profilerPathLinux.c_str() );
        }
        #endif
    }

    void StartCapture()
    {
        #if EE_DEVELOPMENT_TOOLS
        OPTICK_START_CAPTURE();
        #endif
    }

    void StopCapture( FileSystem::Path const& captureSavePath )
    {
        #if EE_DEVELOPMENT_TOOLS
        OPTICK_STOP_CAPTURE();
        OPTICK_SAVE_CAPTURE( captureSavePath.c_str() );
        #endif
    }
}