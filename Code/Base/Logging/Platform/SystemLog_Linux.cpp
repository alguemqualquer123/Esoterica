#if EE_PLATFORM_LINUX
#include "Base/Esoterica.h"
#include <cstdio>
#include <cstdarg>

//-------------------------------------------------------------------------

namespace EE::SystemLog
{
    void TraceMessage( const char* format, ... )
    {
        constexpr size_t bufferSize = 2048;
        char messageBuffer[bufferSize];

        va_list args;
        va_start( args, format );
        int32_t numCharsWritten = vsnprintf( messageBuffer, bufferSize - 1, format, args );
        va_end( args );

        if ( numCharsWritten > 0 && numCharsWritten < (int32_t) bufferSize - 2 )
        {
            messageBuffer[numCharsWritten] = '\n';
            messageBuffer[numCharsWritten + 1] = '\0';
        }

        fputs( messageBuffer, stderr );
        fflush( stderr );
    }
}
#endif
