#if EE_PLATFORM_LINUX
#include "PlatformUtils_Linux.h"
#include "Base/Memory/Memory.h"
#include "Base/Logging/Log.h"

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <errno.h>

//-------------------------------------------------------------------------

namespace EE::Platform::Linux
{
    uint32_t GetProcessID( char const* processName )
    {
        DIR* proc = opendir( "/proc" );
        if ( proc == nullptr )
        {
            return 0;
        }

        struct dirent* entry;
        while ( ( entry = readdir( proc ) ) != nullptr )
        {
            // Only numeric directories are PIDs
            char* end = nullptr;
            long pid = strtol( entry->d_name, &end, 10 );
            if ( end == entry->d_name || *end != '\0' )
            {
                continue;
            }

            char commPath[64];
            snprintf( commPath, sizeof( commPath ), "/proc/%ld/comm", pid );
            FILE* f = fopen( commPath, "r" );
            if ( f == nullptr )
            {
                continue;
            }

            char comm[256] = {};
            if ( fgets( comm, sizeof( comm ), f ) != nullptr )
            {
                // Strip newline
                size_t len = strlen( comm );
                if ( len > 0 && comm[len - 1] == '\n' ) comm[len - 1] = '\0';
                if ( strcmp( comm, processName ) == 0 )
                {
                    fclose( f );
                    closedir( proc );
                    return (uint32_t) pid;
                }
                // Also try matching with basename of processName
                char const* pBase = strrchr( processName, '/' );
                if ( pBase != nullptr && strcmp( comm, pBase + 1 ) == 0 )
                {
                    fclose( f );
                    closedir( proc );
                    return (uint32_t) pid;
                }
            }
            fclose( f );
        }

        closedir( proc );
        return 0;
    }

    uint32_t StartProcess( char const* exePath, char const* cmdLine )
    {
        pid_t pid = fork();
        if ( pid == 0 )
        {
            // Child
            if ( cmdLine != nullptr && cmdLine[0] != '\0' )
            {
                // Use shell to handle cmdLine splitting
                execl( "/bin/sh", "sh", "-c", cmdLine, (char*) nullptr );
                // Fallback: try direct execl
                execl( exePath, exePath, cmdLine, (char*) nullptr );
            }
            else
            {
                execl( exePath, exePath, (char*) nullptr );
            }
            _exit( 127 );
        }
        else if ( pid > 0 )
        {
            return (uint32_t) pid;
        }

        return 0;
    }

    bool KillProcess( uint32_t processID )
    {
        EE_ASSERT( processID != 0 );
        return kill( (pid_t) processID, SIGTERM ) == 0;
    }

    String GetProcessPath( uint32_t processID )
    {
        EE_ASSERT( processID != 0 );
        char linkPath[64];
        snprintf( linkPath, sizeof( linkPath ), "/proc/%u/exe", processID );
        char dest[PATH_MAX] = {};
        ssize_t len = readlink( linkPath, dest, sizeof( dest ) - 1 );
        if ( len > 0 )
        {
            dest[len] = '\0';
            return String( dest );
        }
        return String();
    }

    String GetCurrentModulePath()
    {
        char dest[PATH_MAX] = {};
        ssize_t len = readlink( "/proc/self/exe", dest, sizeof( dest ) - 1 );
        if ( len > 0 )
        {
            dest[len] = '\0';
            return String( dest );
        }
        return String();
    }

    String GetLastErrorMessage()
    {
        return String( strerror( errno ) );
    }

    String GetShortPath( String const& origPath )
    {
        // No short (8.3) concept on Linux — return as-is
        return origPath;
    }

    String GetLongPath( String const& origPath )
    {
        char resolved[PATH_MAX] = {};
        if ( realpath( origPath.c_str(), resolved ) != nullptr )
        {
            return String( resolved );
        }
        return origPath;
    }

    void OpenInExplorer( char const* path )
    {
        EE_ASSERT( path != nullptr && path[0] != 0 );
        // Try xdg-open, then gio, then nautilus
        pid_t pid = fork();
        if ( pid == 0 )
        {
            execlp( "xdg-open", "xdg-open", path, (char*) nullptr );
            execlp( "gio", "gio", "open", path, (char*) nullptr );
            _exit( 0 );
        }
    }
}
#endif
