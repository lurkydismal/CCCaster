#include "overlay.h"

#if 1
#include "_useCallback.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "stdfunc.h"

uint16_t drawElement( const element_t* _element ) {
    uint16_t l_returnValue = 0;

    element_t* l_element = ( element_t* )malloc( sizeof( element_t ) );
    memcpy( l_element, _element, sizeof( element_t ) );

    switch ( _element->type ) {
        case RECTANGLE: {
            l_returnValue =
                _useCallback( "overlay$draw$rectangle", &l_element );

            break;
        }

        case TEXT: {
            l_returnValue = _useCallback( "overlay$draw$text", &l_element );

            break;
        }

        case SPRITE: {
            l_returnValue = _useCallback( "overlay$draw$sprite", &l_element );

            break;
        }

        default: {
            l_returnValue = _useCallback( "overlay$draw$unknown", &l_element );
        }
    }

    free( l_element );

    return ( l_returnValue );
}
