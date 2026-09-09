#pragma once
#include "Base/Esoterica.h"
#include <cstdint>

namespace EE::Math
{
    EE_FORCE_INLINE uint32_t GetMostSignificantBit( uint64_t value )
    {
        if ( value == 0 )
        {
            return 0;
        }
        return 63 - __builtin_clzll( value );
    }
}
