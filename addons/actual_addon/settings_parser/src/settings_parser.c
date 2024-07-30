#include "settings_parser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 255

char** g_sectionLabels;
size_t g_labelCount = 0;
char**** g_sectionContents;

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    printf( "read: %s = %d\n", _fileName, g_labelCount );

    char l_line[ MAX_LINE_LENGTH ];

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        if ( g_labelCount ) {
            for ( size_t _index = 0; _index < g_labelCount; _index++ ) {
                printf( "label: %s\n", g_sectionLabels[ _index ] );
                free( g_sectionLabels[ _index ] );

                for ( size_t _indexContent = 0; _indexContent < (size_t)( g_sectionContents[ _indexContent ][ 0 ][ 0 ] ); _indexContent++ ) {
                    char*** l_label = g_sectionContents[ _index ];
                    char** l_section = g_sectionContents[ _index ][ _indexContent ];
                    char* l_key = g_sectionContents[ _index ][ _indexContent ][ 0 ];
                    char* l_value = g_sectionContents[ _index ][ _indexContent ][ 1 ];

                    printf( "content: %p [ %p ] = %s : %s\n", l_label, l_section, l_key, l_value );

                    free( g_sectionContents[ _index ][ _indexContent ][ 0 ] );
                    free( g_sectionContents[ _index ][ _indexContent ][ 1 ] );
                    free( g_sectionContents[ _index ][ _indexContent ] );
                }

                free( g_sectionContents[ _index ] );
            }

            free( g_sectionLabels );
            free( g_sectionContents );

            g_labelCount = 0;
        }

        g_sectionLabels = ( char** )malloc( 1 * sizeof( char** ) );
        g_sectionContents = ( char**** )malloc( 1 * sizeof( char**** ) );
        g_sectionContents[ 0 ] = ( char*** )malloc( 1 * sizeof( char*** ) );
        size_t l_labelContentCount = 0;

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            bool l_isLabel = false;
            char* l_buffer = ( char* )malloc( MAX_LINE_LENGTH );
            printf( "l_buffer: %p\n", l_buffer );
            size_t l_bufferLength = 0;

            for ( size_t _index = 0; _index < ( MAX_LINE_LENGTH - 1 );
                    _index++ ) {
                const char l_character = l_line[ _index ];

                if ( ( l_character == '#' ) || ( l_character == '\n' ) ||
                        ( l_character == ']' ) ) {
                    l_labelContentCount = 0;

                    break;

                } else if ( l_character == ' ' ) {
                    continue;

                } else if ( ( !l_bufferLength ) && ( l_character == '[' ) ) {
                    l_isLabel = true;
                    g_labelCount++;
                    printf( "[: %d , %d \n", l_isLabel, g_labelCount );

                } else {
                    l_buffer[ l_bufferLength ] = l_character;
                    l_bufferLength++;
                }
            }

            if ( l_bufferLength ) {
                l_buffer[ l_bufferLength ] = '\0';

                const size_t l_labelIndex = ( g_labelCount - 1 );

                printf( "l_buffer 2: %s = %d\n", l_buffer, l_bufferLength );

                if ( l_isLabel ) {
                    if ( l_labelIndex ) {
                        g_sectionLabels = ( char** )realloc(
                                g_sectionLabels, g_labelCount * sizeof( char* ) );
                    }

                    g_sectionLabels[ l_labelIndex ] =
                        ( char* )malloc( l_bufferLength );
                    strcpy( g_sectionLabels[ l_labelIndex ], l_buffer );

                    printf( "g_sectionLabels[ l_labelIndex = %d ]: %s\n", l_labelIndex, g_sectionLabels[ l_labelIndex ] );

                } else if ( ( !l_isLabel ) && ( g_labelCount ) ) {
                    const size_t l_labelIndex = ( g_labelCount - 1 );
                    const size_t l_labelContentIndex = ( l_labelContentCount - 1 );

                    if ( ( l_labelIndex ) && ( !l_labelContentCount ) ) {
                        g_sectionContents = ( char**** )realloc(
                                g_sectionContents, g_labelCount * sizeof( char**** ) );
                    }

                    if ( l_labelContentCount ) {
                        g_sectionContents[ l_labelIndex ] = ( char*** )realloc(
                                g_sectionContents[ l_labelIndex ], l_labelContentCount * sizeof( char*** ) );
                    }

                    g_sectionContents[ l_labelIndex ][ l_labelContentIndex ] =
                        ( char** )malloc( 2 * sizeof( char** ) );
                    printf( "g_sectionContents[ l_labelIndex = %d ][ l_labelContentIndex = %d ]: %p [ %p ]\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ], g_sectionContents[ l_labelIndex ][ l_labelContentIndex ] );

                    for ( size_t _index = 0; _index < ( l_bufferLength - 1 );
                            _index++ ) {
                        if ( l_buffer[ _index ] == '=' ) {
                            char* l_key;
                            char* l_value;

                            printf( "_index = %d\n", _index );

                            const size_t l_keyLength = ( _index );
                            l_key = (char*)malloc( l_keyLength );

                            memcpy( l_key, l_buffer, l_keyLength );
                            l_key[ l_keyLength ] = '\0';

                            printf( "l_key = %s, l_keyLength = %d\n", l_key, l_keyLength );

                            const size_t l_valueLength = ( l_bufferLength - _index - 1 );
                            l_value = (char*)malloc( l_valueLength );
                            l_value[ l_valueLength ] = '\0';

                            memcpy( l_value, l_buffer + l_keyLength + 1, l_valueLength );

                            printf( "l_value = %s, l_valueLength = %d\n", l_value, l_valueLength );

                            // Key
                            g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ] = (char*)malloc( l_keyLength );
                            strcpy( g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ], l_key );

                            printf( "g_sectionContents[ l_labelIndex = %d ][ l_labelContentIndex = %d ][ 0 ]: %s\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ] );

                            // Value
                            g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 1 ] = (char*)malloc( l_valueLength );
                            strcpy( g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 1 ], l_value );

                            printf( "g_sectionContents[ l_labelIndex = %d ][ l_labelContentIndex = %d ][ 1 ]: %s\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ][ 1 ] );

                            free( l_key );
                            free( l_value );

                            break;
                        }
                    }
                }
            }

            free( l_buffer );

            l_labelContentCount++;
        }

        l_returnValue = 0;
    }

    fclose( l_fileHandle );

    return ( l_returnValue );
}

uint16_t writeSettingsToFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    printf( "write: g_labelCount = %d\n", g_labelCount );

    if ( g_labelCount ) {
        // FILE* l_fileHandle = fopen( _fileName, "w" );

        // if ( l_fileHandle ) {
        for ( size_t _index = 0; _index < g_labelCount; _index++ ) {
            printf( "[%s]\n", g_sectionLabels[ _index ] );
            printf( "%s = %s\n", g_sectionContents[ _index ][ 0 ], g_sectionContents[ _index ][ 1 ] );
        }

        // fwrite( _string, sizeof( _string[ 0 ] ), _stringLength, l_fileHandle );

        l_returnValue = 0;
        // }

        // fclose( l_fileHandle );
    }

    return ( l_returnValue );
}
