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

    switch ( _element->type ) {
        default: {
            l_returnValue = _useCallback( "overlay$interact$unknown", _element,
                                          _activeMappedKeys, _activeKeys );
        }
    }

    return ( l_returnValue );
}
