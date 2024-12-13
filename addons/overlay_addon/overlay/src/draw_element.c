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

    char* l_callbackName = strdup( "overlay$draw$" );

    concatBeforeAndAfterString( &l_callbackName, "", _element->type );

    l_returnValue = _useCallback( l_callbackName, l_element );

    free( l_callbackName );

    free( l_element );

    return ( l_returnValue );
}
