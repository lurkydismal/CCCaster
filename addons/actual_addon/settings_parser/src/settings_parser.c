#include "settings_parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "_useCallback.h"

#define MAX_LINE_LENGTH 255

char** g_sectionLabels;
size_t g_labelCount = 0;
char*** g_sectionContents;

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    char l_line[ MAX_LINE_LENGTH ];

    if ( g_labelCount ) {
        for ( size_t _index = 0; _index < g_labelCount; _index++ ) {
            free( g_sectionLabels[ _index ] );
        }

        free( g_sectionLabels );

        g_labelCount = 0;
    }

    g_sectionLabels = ( char** )malloc( 1 * sizeof( char* ) );
    g_sectionContents = ( char*** )malloc( 1 * sizeof( char* ) );

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            bool l_isLabel = false;
            char* l_buffer = ( char* )malloc( MAX_LINE_LENGTH );
            size_t l_bufferLength = 0;

            for ( size_t _index = 0; _index < ( MAX_LINE_LENGTH - 1 );
                  _index++ ) {
                const char l_character = l_line[ _index ];

                if ( ( l_character == '#' ) || ( l_character == '\n' ) ||
                     ( l_character == ']' ) ) {
                    break;

                } else if ( l_character == ' ' ) {
                    continue;

                } else if ( ( !l_bufferLength ) && ( l_character == '[' ) ) {
                    l_isLabel = true;
                    g_labelCount++;

                } else {
                    l_buffer[ l_bufferLength ] = l_character;
                    l_bufferLength++;
                }
            }

            l_buffer[ l_bufferLength ] = '\0';

            const size_t l_labelIndex = ( g_labelCount - 1 );

            if ( l_isLabel ) {
                if ( l_labelIndex ) {
                    g_sectionLabels = ( char** )realloc(
                        g_sectionLabels, g_labelCount * sizeof( char* ) );
                }

                g_sectionLabels[ l_labelIndex ] =
                    ( char* )malloc( l_bufferLength );
                memcpy( g_sectionLabels[ l_labelIndex ], l_buffer,
                        l_bufferLength );

            } else if ( ( !l_isLabel ) && ( g_labelCount ) ) {
                if ( l_labelIndex ) {
                    g_sectionContents = ( char*** )realloc(
                        g_sectionContents, g_labelCount * sizeof( char* ) );
                }

                g_sectionContents[ l_labelIndex ] =
                    ( char** )malloc( 2 * sizeof( char* ) );

                for ( size_t _index = 0; _index < ( l_bufferLength - 1 );
                      _index++ ) {
                    if ( l_buffer[ _index ] == '=' ) {
                        char* l_key;
                        char* l_value;
#if 0

                        const size_t l_keyLength = ( _index + 1 );
                        l_key = (char*)malloc( l_keyLength );

                        memcpy( l_key, l_buffer, l_keyLength );
                        l_key[ 0 ] = '\0';

                        const size_t l_valueLength = ( l_bufferLength - _index - 1 );
                        l_value = (char*)malloc( l_valueLength );
                        l_value[ l_valueLength ] = '\0';

                        memcpy( l_value, l_buffer + l_keyLength, l_valueLength );

                        // Key
                        g_sectionContents[ l_labelIndex ][ 0 ] = (char*)malloc( l_keyLength );
                        memcpy( g_sectionContents[ l_labelIndex ][ 0 ], l_key, l_keyLength );

                        // Value
                        g_sectionContents[ l_labelIndex ][ 0 ] = (char*)malloc( l_valueLength );
                        memcpy( g_sectionContents[ l_labelIndex ][ 1 ], l_value, l_valueLength );

                        free( l_key );
                        free( l_value );
#endif

                        break;
                    }
                }
            }

            free( l_buffer );
        }

        l_returnValue = 0;
    }

    fclose( l_fileHandle );

    return ( l_returnValue );
}

uint16_t writeSettingsToFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        // fwrite( _string, sizeof( _string[ 0 ] ), _stringLength, l_fileHandle
        // );

        l_returnValue = 0;
    }

    fclose( l_fileHandle );

    return ( l_returnValue );
}
