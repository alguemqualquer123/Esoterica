#if EE_PLATFORM_LINUX
#include "../FileSystemPath.h"
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    char const Path::s_pathDelimiter = '/';
    constexpr static size_t g_maxPathBufferLength = PATH_MAX;

    //-------------------------------------------------------------------------

    void Path::EnsureCorrectPathStringFormat()
    {
        struct stat st;
        if ( stat( m_fullpath.c_str(), &st ) != 0 )
        {
            return;
        }

        bool const isPathADirectory = S_ISDIR( st.st_mode );

        if ( isPathADirectory && !IsDirectoryPath() )
        {
            m_fullpath += s_pathDelimiter;
            UpdatePathInternals();
        }
        else if ( !isPathADirectory && IsDirectoryPath() )
        {
            m_fullpath.pop_back();
            UpdatePathInternals();
        }
    }

    bool Path::GetFullPathString( char const* pPath, String& outPath )
    {
        if ( pPath != nullptr && pPath[0] != 0 )
        {
            char resolved[PATH_MAX] = {};
            // realpath resolves symlinks and returns absolute path; if file doesn't exist, use manual resolution
            if ( realpath( pPath, resolved ) != nullptr )
            {
                outPath = resolved;

                struct stat st;
                if ( stat( resolved, &st ) == 0 && S_ISDIR( st.st_mode ) && outPath.back() != s_pathDelimiter )
                {
                    outPath += s_pathDelimiter;
                }
                return true;
            }
            else
            {
                // Fallback: construct absolute path manually if file doesn't exist yet
                if ( pPath[0] == '/' )
                {
                    outPath = pPath;
                }
                else
                {
                    char cwd[PATH_MAX] = {};
                    if ( getcwd( cwd, sizeof( cwd ) ) != nullptr )
                    {
                        outPath = String( cwd ) + "/" + pPath;
                    }
                    else
                    {
                        outPath = pPath;
                    }
                }
                return true;
            }
        }

        outPath.clear();
        return false;
    }

    bool Path::GetCorrectCaseForPath( char const* pPath, String& outPath )
    {
        // Linux is case-sensitive — return as-is
        // We still try to resolve symlinks via realpath for canonical form
        char resolved[PATH_MAX] = {};
        if ( realpath( pPath, resolved ) != nullptr )
        {
            outPath = resolved;
            return true;
        }

        outPath = pPath;
        // Consider success if path exists, otherwise still return false to indicate fallback
        struct stat st;
        return stat( pPath, &st ) == 0;
    }
}
#endif
