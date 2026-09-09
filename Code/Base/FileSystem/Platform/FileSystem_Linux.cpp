#if EE_PLATFORM_LINUX
#include "../FileSystem.h"
#include "Base/Platform/PlatformUtils_Linux.h"
#include "Base/Encoding/Hash.h"
#include "Base/Math/Math.h"
#include "Base/Logging/Log.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <limits.h>

//-------------------------------------------------------------------------

namespace EE::FileSystem
{
    Path GetCurrentProcessPath()
    {
        return Path( EE::Platform::Linux::GetCurrentModulePath() ).GetParentDirectory();
    }

    bool Exists( char const* pPath )
    {
        struct stat st;
        return stat( pPath, &st ) == 0;
    }

    bool IsReadOnly( char const* pPath )
    {
        struct stat st;
        if ( stat( pPath, &st ) != 0 ) return false;
        // Check if owner write bit is missing
        return ( st.st_mode & S_IWUSR ) == 0;
    }

    bool IsExistingFile( char const* pPath )
    {
        struct stat st;
        if ( stat( pPath, &st ) != 0 ) return false;
        return S_ISREG( st.st_mode );
    }

    bool IsExistingDirectory( char const* pPath )
    {
        struct stat st;
        if ( stat( pPath, &st ) != 0 ) return false;
        return S_ISDIR( st.st_mode );
    }

    bool IsFileReadOnly( char const* pPath )
    {
        struct stat st;
        if ( stat( pPath, &st ) != 0 ) return false;
        if ( !S_ISREG( st.st_mode ) ) return false;
        return ( st.st_mode & S_IWUSR ) == 0;
    }

    uint64_t GetFileModifiedTime( char const* path )
    {
        struct stat st;
        if ( stat( path, &st ) == 0 )
        {
            // Use mtime as uint64_t (seconds + nanoseconds if available)
            #ifdef __linux__
            return (uint64_t) st.st_mtim.tv_sec * 1000000000ULL + (uint64_t) st.st_mtim.tv_nsec;
            #else
            return (uint64_t) st.st_mtime;
            #endif
        }
        return 0;
    }

    //-------------------------------------------------------------------------

    bool WriteFileToDisk( char const* pPath, void const* pData, size_t size, bool overwrite, bool flushToDisk )
    {
        int flags = O_WRONLY | O_CREAT;
        flags |= overwrite ? O_TRUNC : O_EXCL;
        int fd = open( pPath, flags, 0644 );
        if ( fd < 0 )
        {
            String const errorString = Platform::Linux::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "WriteFile", "Failed to open file for write: %s, Error: %s", pPath, errorString.c_str() );
            return false;
        }

        bool success = true;
        uint8_t const* pWritePtr = static_cast<uint8_t const*>( pData );
        size_t remainingBytes = size;

        while ( remainingBytes > 0 )
        {
            ssize_t written = write( fd, pWritePtr, remainingBytes );
            if ( written <= 0 )
            {
                String const errorString = Platform::Linux::GetLastErrorMessage();
                EE_LOG_ERROR( LogCategory::FileSystem, "WriteFile", "Failed to write file: %s, Error: %s", pPath, errorString.c_str() );
                success = false;
                break;
            }
            pWritePtr += written;
            remainingBytes -= (size_t) written;
        }

        if ( success && flushToDisk )
        {
            success = ( fsync( fd ) == 0 );
        }

        close( fd );
        return success;
    }

    bool ReadBinaryFile( char const* pPath, Blob& fileData )
    {
        EE_ASSERT( pPath != nullptr );

        int fd = open( pPath, O_RDONLY );
        if ( fd < 0 )
        {
            String errorString = Platform::Linux::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to open file for read: %s, Error: %s", pPath, errorString.c_str() );
            return false;
        }

        struct stat st;
        if ( fstat( fd, &st ) != 0 )
        {
            String const errorString = Platform::Linux::GetLastErrorMessage();
            EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to get file size for: %s, Error: %s", pPath, errorString.c_str() );
            close( fd );
            return false;
        }

        size_t const fileSize = (size_t) st.st_size;
        fileData.resize( fileSize );

        static constexpr size_t defaultReadBufferSize = 65536;
        size_t remainingBytesToRead = fileSize;
        uint8_t* pBuffer = fileData.data();

        while ( remainingBytesToRead != 0 )
        {
            size_t const numBytesToRead = Math::Min( defaultReadBufferSize, remainingBytesToRead );
            ssize_t bytesRead = read( fd, pBuffer, numBytesToRead );
            if ( bytesRead <= 0 )
            {
                fileData.clear();
                String const errorString = Platform::Linux::GetLastErrorMessage();
                EE_LOG_ERROR( LogCategory::FileSystem, "ReadBinaryFile", "Failed to read file: %s, Error: %s", pPath, errorString.c_str() );
                close( fd );
                return false;
            }
            pBuffer += bytesRead;
            remainingBytesToRead -= (size_t) bytesRead;
        }

        close( fd );
        return true;
    }
}
#endif
