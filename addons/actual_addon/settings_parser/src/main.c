#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 255

char** g_sectionLabels;
size_t g_labelCount = 1;
char**** g_sectionContents;

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    printf( "read: %s = %lu\n", _fileName, g_labelCount );

    char l_line[ MAX_LINE_LENGTH ];

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        if ( g_labelCount > 1 ) {
            for ( size_t _index = 1; _index < g_labelCount; _index++ ) {
                printf( "label: %s\n", g_sectionLabels[ _index ] );
                free( g_sectionLabels[ _index ] );

                printf( "length: %d\n", *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) );

                for ( size_t _indexContent = 1; _indexContent < (size_t)( *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) ); _indexContent++ ) {
                    char*** l_label = g_sectionContents[ _index ];
                    char** l_section = g_sectionContents[ _index ][ _indexContent ];
                    char* l_key = g_sectionContents[ _index ][ _indexContent ][ 0 ];
                    char* l_value = g_sectionContents[ _index ][ _indexContent ][ 1 ];

                    printf( "content: %p [ %p ] = %s : %s\n", l_label, l_section, l_key, l_value );

                    free( g_sectionContents[ _index ][ _indexContent ][ 0 ] );
                    free( g_sectionContents[ _index ][ _indexContent ][ 1 ] );
                    free( g_sectionContents[ _index ][ _indexContent ] );
                }

                free( g_sectionContents[ _index ][ 0 ][ 0 ] );
                free( g_sectionContents[ _index ][ 0 ] );
                free( g_sectionContents[ _index ] );
            }

            free( g_sectionLabels );
            free( g_sectionContents );

            g_labelCount = 1;
        }

        g_sectionLabels = ( char** )malloc( 1 * sizeof( char** ) );
        g_sectionContents = ( char**** )malloc( 1 * sizeof( char**** ) );
        g_sectionContents[ 0 ] = ( char*** )malloc( 2 * sizeof( char*** ) );
        g_sectionContents[ 0 ][ 0 ] = ( char** )malloc( 1 * sizeof( char** ) );
        g_sectionContents[ 0 ][ 0 ][ 0 ] = ( char* )malloc( 1 );

        *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) = 0;

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
                    break;

                } else if ( l_character == ' ' ) {
                    continue;

                } else if ( ( !l_bufferLength ) && ( l_character == '[' ) ) {
                    l_labelContentCount = 0;

                    l_isLabel = true;
                    g_labelCount++;
                    printf( "[: %d , %lu \n", l_isLabel, g_labelCount );

                } else {
                    l_buffer[ l_bufferLength ] = l_character;
                    l_bufferLength++;
                }
            }

            if ( l_bufferLength ) {
                l_buffer[ l_bufferLength ] = '\0';

                const size_t l_labelIndex = ( g_labelCount - 1 );

                printf( "l_buffer 2: %s = %lu\n", l_buffer, l_bufferLength );

                if ( l_isLabel ) {
                    printf( "isLabel\n" );

                    if ( l_labelIndex ) {
                        g_sectionLabels = ( char** )realloc(
                                g_sectionLabels, g_labelCount * sizeof( char* ) );
                    }

                    g_sectionLabels[ l_labelIndex ] =
                        ( char* )malloc( l_bufferLength );
                    strcpy( g_sectionLabels[ l_labelIndex ], l_buffer );

                    printf( "g_sectionLabels[ l_labelIndex = %lu ]: %s\n", l_labelIndex, g_sectionLabels[ l_labelIndex ] );

                } else if ( ( !l_isLabel ) && ( g_labelCount ) ) {
                    printf( "isContent\n" );

                    l_labelContentCount++;

                    const size_t l_labelIndex = ( g_labelCount - 1 );
                    const size_t l_labelContentIndex = ( l_labelContentCount - 1 );
                    printf( "%lu %lu : %lu %lu\n", g_labelCount, l_labelIndex, l_labelContentCount, l_labelContentIndex );

                    if ( ( l_labelIndex ) && ( !l_labelContentCount ) ) {
                        printf( "1\n" );
                        g_sectionContents = ( char**** )realloc(
                                g_sectionContents, g_labelCount * sizeof( char**** ) );
                    }

                    if ( l_labelContentCount ) {
                        printf( "2\n" );
                        g_sectionContents[ l_labelIndex ] = ( char*** )realloc(
                                g_sectionContents[ l_labelIndex ], l_labelContentCount * sizeof( char*** ) );

                        printf( "3\n" );
                        *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) = l_labelContentCount;
                    }

                    g_sectionContents[ l_labelIndex ][ l_labelContentIndex ] =
                        ( char** )malloc( 2 * sizeof( char** ) );
                    printf( "g_sectionContents[ l_labelIndex = %lu ][ l_labelContentIndex = %lu ]: %p [ %p ]\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ], g_sectionContents[ l_labelIndex ][ l_labelContentIndex ] );

                    for ( size_t _index = 0; _index < ( l_bufferLength - 1 );
                            _index++ ) {
                        if ( l_buffer[ _index ] == '=' ) {
                            char* l_key;
                            char* l_value;

                            printf( "_index = %lu\n", _index );

                            const size_t l_keyLength = ( _index );
                            l_key = (char*)malloc( l_keyLength );

                            memcpy( l_key, l_buffer, l_keyLength );
                            l_key[ l_keyLength ] = '\0';

                            printf( "l_key = %s, l_keyLength = %lu\n", l_key, l_keyLength );

                            const size_t l_valueLength = ( l_bufferLength - _index - 1 );
                            l_value = (char*)malloc( l_valueLength );
                            l_value[ l_valueLength ] = '\0';

                            memcpy( l_value, l_buffer + l_keyLength + 1, l_valueLength );

                            printf( "l_value = %s, l_valueLength = %lu\n", l_value, l_valueLength );

                            // Key
                            g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ] = (char*)malloc( l_keyLength );
                            strcpy( g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ], l_key );

                            printf( "g_sectionContents[ l_labelIndex = %lu ][ l_labelContentIndex = %lu ][ 0 ]: %s\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 0 ] );

                            // Value
                            g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 1 ] = (char*)malloc( l_valueLength );
                            strcpy( g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 1 ], l_value );

                            printf( "g_sectionContents[ l_labelIndex = %lu ][ l_labelContentIndex = %lu ][ 1 ]: %s\n", l_labelIndex, l_labelContentIndex, g_sectionContents[ l_labelIndex ][ l_labelContentIndex ][ 1 ] );

                            free( l_key );
                            free( l_value );

                            break;
                        }
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

    printf( "write: g_labelCount = %lu\n", g_labelCount );

    if ( g_labelCount > 1 ) {
        FILE* l_fileHandle = fopen( _fileName, "w" );

        if ( l_fileHandle ) {
            for ( size_t _index = 1; _index < g_labelCount; _index++ ) {
                {
                    const char l_message[] = "[%s]\n";
                    const size_t l_messageLength = snprintf( NULL, 0, l_message, g_sectionLabels[ _index ] );
                    char l_buffer[ l_messageLength + 1 ];
                    snprintf( l_buffer, sizeof( l_buffer ), l_message, g_sectionLabels[ _index ] );

                    printf( "%s", l_buffer );
                    fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), sizeof( l_buffer ), l_fileHandle );
                }

                printf( "length: %d\n", *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) );

                for ( size_t _indexContent = 1; _indexContent < (size_t)( *( g_sectionContents[ 0 ][ 0 ][ 0 ] ) ); _indexContent++ ) {
                    char*** l_label = g_sectionContents[ _index ];
                    char** l_section = g_sectionContents[ _index ][ _indexContent ];
                    char* l_key = g_sectionContents[ _index ][ _indexContent ][ 0 ];
                    char* l_value = g_sectionContents[ _index ][ _indexContent ][ 1 ];

                    {
                        const char l_message[] = "%s = %s\n";
                        const size_t l_messageLength = snprintf( NULL, 0, l_message, l_key, l_value );
                        char l_buffer[ l_messageLength + 1 ];
                        snprintf( l_buffer, sizeof( l_buffer ), l_message, l_key, l_value );

                        printf( "content: %p [ %p ] : %s", l_label, l_section, l_buffer );
                        fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), sizeof( l_buffer ), l_fileHandle );
                    }
                }
            }

            l_returnValue = 0;
        }

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

int main( const int _argumentCount, const char* const* _argumentArray ) {
    printf( "begin\n" );

    if ( _argumentCount < 3 ) {
        printf( "no arguments\n" );

        goto EXIT;
    }

    readSettingsFromFile( _argumentArray[ 1 ] );

    writeSettingsToFile( _argumentArray[ 2 ] );

    readSettingsFromFile( _argumentArray[ 2 ] );

    writeSettingsToFile( _argumentArray[ 3 ] );

EXIT:
    printf( "end\n" );

    return ( 0 );
}
