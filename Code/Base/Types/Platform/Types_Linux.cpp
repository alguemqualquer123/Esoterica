#if EE_PLATFORM_LINUX
#include "../UUID.h"
#include <uuid/uuid.h>
#include <strings.h>
#include <cstring>

//-------------------------------------------------------------------------

namespace EE
{
    UUID UUID::GenerateID()
    {
        UUID newID;
        static_assert( sizeof( uuid_t ) == sizeof( UUID ), "Size mismatch for EE UUID vs uuid_t" );
        uuid_generate( reinterpret_cast<uuid_t&>( newID ) );
        return newID;
    }

    //-------------------------------------------------------------------------

    namespace StringUtils
    {
        int32_t CompareInsensitive( char const* pStr0, char const* pStr1 )
        {
            return strcasecmp( pStr0, pStr1 );
        }

        int32_t CompareInsensitive( char const* pStr0, char const* pStr1, size_t n )
        {
            return strncasecmp( pStr0, pStr1, n );
        }
    }
}
#endif
