// XGetInputFocus(xdo->xdpy, window_ret, &unused_revert_ret);

/* Arbitrary window property retrieval
 * slightly modified version from xprop.c from Xorg */
unsigned char* xdo_get_window_property_by_atom( const Display* dpy,
                                                Window window,
                                                Atom atom,
                                                long* nitems,
                                                Atom* type,
                                                int* size ) {
    Atom actual_type;
    int actual_format;
    unsigned long _nitems;
    /*unsigned long nbytes;*/
    unsigned long bytes_after; /* unused */
    unsigned char* prop;
    int status;

    status = XGetWindowProperty( dpy, window, atom, 0, ( ~0L ), False,
                                 AnyPropertyType, &actual_type, &actual_format,
                                 &_nitems, &bytes_after, &prop );
    if ( status == BadWindow ) {
        fprintf( stderr, "window id # 0x%lx does not exists!", window );
        return NULL;
    }
    if ( status != Success ) {
        fprintf( stderr, "XGetWindowProperty failed!" );
        return NULL;
    }

    /*
     *if (actual_format == 32)
     *  nbytes = sizeof(long);
     *else if (actual_format == 16)
     *  nbytes = sizeof(short);
     *else if (actual_format == 8)
     *  nbytes = 1;
     *else if (actual_format == 0)
     *  nbytes = 0;
     */

    if ( nitems != NULL ) {
        *nitems = _nitems;
    }

    if ( type != NULL ) {
        *type = actual_type;
    }

    if ( size != NULL ) {
        *size = actual_format;
    }
    return prop;
}

int _xdo_ewmh_is_supported( const Display* dpy, const char* feature ) {
    Atom type = 0;
    long nitems = 0L;
    int size = 0;
    Atom* results = NULL;
    long i = 0;

    Window root;
    Atom request;
    Atom feature_atom;

    request = XInternAtom( dpy, "_NET_SUPPORTED", False );
    feature_atom = XInternAtom( dpy, feature, False );
    root = XDefaultRootWindow( dpy );

    results = ( Atom* )xdo_get_window_property_by_atom( dpy, root, request,
                                                        &nitems, &type, &size );
    for ( i = 0L; i < nitems; i++ ) {
        if ( results[ i ] == feature_atom ) {
            free( results );
            return True;
        }
    }
    free( results );
}

int xdo_get_active_window( const Display* dpy ) {
    Window window_ret;

    Atom type;
    int size;
    long nitems;
    unsigned char* data;
    Atom request;
    Window root;

    if ( _xdo_ewmh_is_supported( dpy, "_NET_ACTIVE_WINDOW" ) == False ) {
        fprintf( stderr,
                 "Your windowmanager claims not to support _NET_ACTIVE_WINDOW, "
                 "so the attempt to query the active window aborted.\n" );
        return XDO_ERROR;
    }

    request = XInternAtom( dpy, "_NET_ACTIVE_WINDOW", False );
    root = XDefaultRootWindow( dpy );
    data = xdo_get_window_property_by_atom( dpy, root, request, &nitems, &type,
                                            &size );

    if ( nitems > 0 ) {
        window_ret = *( ( Window* )data );
    } else {
        window_ret = 0;
    }
    free( data );

    return ( window_ret );
}

///////////////
/// @brief Get \c Window to needed window by name on \c Display .
/// @details Recursive function
/// @param[in] _display \c Display pointer.
/// @param[in] _window default root window.
/// @param[in] _windowName Window name.
/// @return \c Window information.
///////////////
static Window windowSearch( Display* _display,
                            Window _window,
                            const regex_t* _windowNameRegExp ) {
    //! <b>[declare]</b>
    /// @code{.cpp}
    Window l_window = 0;
    Window l_root;
    Window l_parent;
    Window* l_children;
    uint32_t l_childrenCount;
    int l_windowNamesCount = 0;
    XTextProperty l_xTextProperty;
    char** l_windowNamesList = NULL;
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[check]</b>
    /// Compare root window name to received.
    /// @code{.cpp}
    XGetWMName( _display, _window, &l_xTextProperty );

    if ( l_xTextProperty.nitems > 0 ) {
        Xutf8TextPropertyToTextList( _display, &l_xTextProperty,
                                     &l_windowNamesList, &l_windowNamesCount );

        for ( int _windowNameIndex = 0; _windowNameIndex < l_windowNamesCount;
              _windowNameIndex++ ) {
            if ( regexec( _windowNameRegExp,
                          l_windowNamesList[ _windowNameIndex ], 0, NULL,
                          0 ) == 0 ) {
                XFreeStringList( l_windowNamesList );
                XFree( l_xTextProperty.value );

                return ( _window );
            }
        }
    }

    XFreeStringList( l_windowNamesList );
    XFree( l_xTextProperty.value );
    /// @endcode
    //! <b>[check]</b>

    //! <b>[check_next]</b>
    /// Compare all window names to received.
    /// @code{.cpp}
    if ( XQueryTree( _display, _window, &l_root, &l_parent, &l_children,
                     &l_childrenCount ) ) {
        for ( uint32_t _childrenIndex = 0; _childrenIndex < l_childrenCount;
              ++_childrenIndex ) {
            l_window = windowSearch( _display, l_children[ _childrenIndex ],
                                     _windowNameRegExp );

            if ( l_window ) {
                break;
            }
        }

        XFree( l_children );
    }
    /// @endcode
    //! <b>[check_next]</b>

    //! <b>[return]</b>
    /// End of function.
    /// @code{.cpp}
    return ( l_window );
    /// @endcode
    //! <b>[return]</b>
}

///////////////
/// @brief Get \c Window to needed window by name.
/// @param[in] _windowName Window name.
/// @return \c Window information.
///////////////
int main( void ) {
    Display* l_display = XOpenDisplay( NULL );
    regex_t l_windowNameRegExp;

    regcomp( &l_windowNameRegExp, _windowName.c_str(),
             REG_EXTENDED | REG_ICASE );
    /// @endcode
    //! <b>[declare]</b>

    //! <b>[search]</b>
    /// Get \c Window by window name.
    /// @code{.cpp}
    Window l_window = windowSearch( l_display, XDefaultRootWindow( l_display ),
                                    &l_windowNameRegExp );
    /// @endcode
    //! <b>[search]</b>

    //! <b>[close]</b>
    /// @code{.cpp}
    regfree( &l_windowNameRegExp );
    XCloseDisplay( l_display );
    /// @endcode
    //! <b>[close]</b>

    return ( 0 );
}