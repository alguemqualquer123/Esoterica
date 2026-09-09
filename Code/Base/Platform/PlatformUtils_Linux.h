#pragma once
#if EE_PLATFORM_LINUX

#include "Base/_Module/API.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------
// Platform Specific Helpers/Functions — Linux
//-------------------------------------------------------------------------

namespace EE::Platform::Linux
{
    // File system
    //-------------------------------------------------------------------------

    EE_BASE_API String GetShortPath( String const& origPath );
    EE_BASE_API String GetLongPath( String const& origPath );

    // Processes
    //-------------------------------------------------------------------------

    EE_BASE_API uint32_t GetProcessID( char const* processName );
    EE_BASE_API String GetProcessPath( uint32_t processID );
    EE_BASE_API String GetCurrentModulePath();
    EE_BASE_API String GetLastErrorMessage();

    EE_BASE_API uint32_t StartProcess( char const* exePath, char const* cmdLine = nullptr );
    EE_BASE_API bool KillProcess( uint32_t processID );

    inline bool IsProcessRunning( char const* processName, uint32_t* pProcessID ) { return GetProcessID( processName ) != 0; }

    EE_BASE_API void OpenInExplorer( char const* path );
}

// Alias for cross-platform code that historically used Platform::Win32
namespace EE::Platform::Win32 = EE::Platform::Linux;

#endif
