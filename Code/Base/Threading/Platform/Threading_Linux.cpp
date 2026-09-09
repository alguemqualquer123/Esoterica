#if EE_PLATFORM_LINUX
#include "Base/Threading/Threading.h"
#include "Base/Esoterica.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace EE::Threading
{
    struct SyncEventImpl
    {
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_signaled = false;
    };
}

//-------------------------------------------------------------------------

namespace EE::Threading
{
    ProcessorInfo GetProcessorInfo()
    {
        ProcessorInfo procInfo;
        long nprocs = sysconf( _SC_NPROCESSORS_ONLN );
        if ( nprocs < 1 ) nprocs = 1;

        // Try to distinguish physical vs logical via /proc/cpuinfo or sysfs
        // Fallback: assume logical == online, physical == logical / 2 if hyperthreading detected
        procInfo.m_numLogicalCores = (uint16_t) nprocs;

        // Attempt to count physical cores via /sys/devices/system/cpu/cpu*/topology/core_id
        // Simple heuristic: count unique core_ids
        // If fails, fallback to logical/2 or logical
        FILE* f = fopen( "/proc/cpuinfo", "r" );
        if ( f != nullptr )
        {
            int physicalCount = 0;
            char line[512];
            while ( fgets( line, sizeof( line ), f ) != nullptr )
            {
                if ( strncmp( line, "cpu cores", 9 ) == 0 )
                {
                    int cores = 0;
                    if ( sscanf( line, "cpu cores : %d", &cores ) == 1 && cores > physicalCount )
                    {
                        physicalCount = cores;
                    }
                }
            }
            fclose( f );
            if ( physicalCount > 0 )
            {
                procInfo.m_numPhysicalCores = (uint16_t) physicalCount;
                // If system has multiple physical CPUs, multiply — but proc/cpuinfo repeats per logical
                // So if logical > physical, keep as is; otherwise assume no HT
                if ( procInfo.m_numLogicalCores < procInfo.m_numPhysicalCores )
                {
                    procInfo.m_numPhysicalCores = procInfo.m_numLogicalCores;
                }
            }
            else
            {
                procInfo.m_numPhysicalCores = procInfo.m_numLogicalCores;
            }
        }
        else
        {
            procInfo.m_numPhysicalCores = procInfo.m_numLogicalCores;
        }

        return procInfo;
    }

    //-------------------------------------------------------------------------

    ThreadID GetCurrentThreadID()
    {
        return (ThreadID) (uintptr_t) pthread_self();
    }

    void SetCurrentThreadName( char const* pName )
    {
        EE_ASSERT( pName != nullptr );
        pthread_setname_np( pthread_self(), pName );
    }

    //-------------------------------------------------------------------------

    SyncEvent::SyncEvent()
        : m_pNativeHandle( nullptr )
    {
        auto* pImpl = new SyncEventImpl();
        m_pNativeHandle = pImpl;
    }

    SyncEvent::~SyncEvent()
    {
        if ( m_pNativeHandle != nullptr )
        {
            auto* pImpl = reinterpret_cast<SyncEventImpl*>( m_pNativeHandle );
            delete pImpl;
            m_pNativeHandle = nullptr;
        }
    }

    void SyncEvent::Signal()
    {
        EE_ASSERT( m_pNativeHandle != nullptr );
        auto* pImpl = reinterpret_cast<SyncEventImpl*>( m_pNativeHandle );
        {
            std::lock_guard<std::mutex> lock( pImpl->m_mutex );
            pImpl->m_signaled = true;
        }
        pImpl->m_cv.notify_all();
    }

    void SyncEvent::Reset()
    {
        EE_ASSERT( m_pNativeHandle != nullptr );
        auto* pImpl = reinterpret_cast<SyncEventImpl*>( m_pNativeHandle );
        std::lock_guard<std::mutex> lock( pImpl->m_mutex );
        pImpl->m_signaled = false;
    }

    void SyncEvent::Wait() const
    {
        EE_ASSERT( m_pNativeHandle != nullptr );
        auto* pImpl = reinterpret_cast<SyncEventImpl*>( const_cast<void*>( m_pNativeHandle ) );
        std::unique_lock<std::mutex> lock( pImpl->m_mutex );
        pImpl->m_cv.wait( lock, [pImpl] { return pImpl->m_signaled; } );
    }

    void SyncEvent::Wait( Milliseconds maxWaitTime ) const
    {
        EE_ASSERT( m_pNativeHandle != nullptr );
        auto* pImpl = reinterpret_cast<SyncEventImpl*>( const_cast<void*>( m_pNativeHandle ) );
        std::unique_lock<std::mutex> lock( pImpl->m_mutex );
        pImpl->m_cv.wait_for( lock, std::chrono::milliseconds( (int64_t) maxWaitTime ), [pImpl] { return pImpl->m_signaled; } );
    }
}
#endif
