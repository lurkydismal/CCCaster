#include "overlay.h"

#if 1
#include "_useCallback.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "stdfunc.h"

uint16_t interactElement( const element_t* _element,
                          char*** _activeMappedKeys,
                          char*** _activeKeys ) {
    uint16_t l_returnValue = 0;

    char* l_callbackName = strdup( "overlay$interact$" );

    concatBeforeAndAfterString( &l_callbackName, "",
                                g_elementTypesAsString[ _element->type ] );

    l_returnValue = _useCallback( l_callbackName, _element, _activeMappedKeys,
                                  _activeKeys );

    free( l_callbackName );

    return ( l_returnValue );
}
