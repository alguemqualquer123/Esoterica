#if EE_PLATFORM_LINUX
#include "PlatformUtils_Linux.h"
#include "Base/Esoterica.h"
#include "Base/Types/Arrays.h"
#include "Base/Logging/SystemLog.h"

#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <sys/stat.h>
#include <time.h>

//-------------------------------------------------------------------------

namespace EE::Platform
{
    //-------------------------------------------------------------------------
    // Crash Handling — signal based
    //-------------------------------------------------------------------------

    namespace
    {
        void GenerateStackTrace()
        {
            void* buffer[128];
            int nptrs = backtrace( buffer, 128 );
            char** strings = backtrace_symbols( buffer, nptrs );
            if ( strings != nullptr )
            {
                for ( int i = 0; i < nptrs; ++i )
                {
                    EE_TRACE_MSG( "%s", strings[i] );
                }
                free( strings );
            }
        }

        void SignalHandler( int sig, siginfo_t* info, void* context )
        {
            EE_LOG_ERROR( LogCategory::General, "Platform/Linux", "Signal %d received at %p", sig, info->si_addr );
            GenerateStackTrace();
            SystemLog::SaveToFile();

            // Restore default and re-raise
            signal( sig, SIG_DFL );
            raise( sig );
        }

        struct sigaction g_prevActionILL  = {};
        struct sigaction g_prevActionSEGV = {};
        struct sigaction g_prevActionBUS  = {};
        struct sigaction g_prevActionABRT = {};
        struct sigaction g_prevActionFPE  = {};
    }

    //-------------------------------------------------------------------------
    // Platform
    //-------------------------------------------------------------------------

    void Initialize()
    {
        struct sigaction sa = {};
        sa.sa_sigaction = SignalHandler;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigemptyset( &sa.sa_mask );

        sigaction( SIGILL,  &sa, &g_prevActionILL );
        sigaction( SIGSEGV, &sa, &g_prevActionSEGV );
        sigaction( SIGBUS,  &sa, &g_prevActionBUS );
        sigaction( SIGABRT, &sa, &g_prevActionABRT );
        sigaction( SIGFPE,  &sa, &g_prevActionFPE );
    }

    void Shutdown()
    {
        sigaction( SIGILL,  &g_prevActionILL,  nullptr );
        sigaction( SIGSEGV, &g_prevActionSEGV, nullptr );
        sigaction( SIGBUS,  &g_prevActionBUS,  nullptr );
        sigaction( SIGABRT, &g_prevActionABRT, nullptr );
        sigaction( SIGFPE,  &g_prevActionFPE,  nullptr );
    }
}
#endif
