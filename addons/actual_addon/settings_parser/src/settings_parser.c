#include "settings_parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum { KEY, VALUE } content_t;

static char** g_labels;
static size_t g_labelCount = 0;
static char**** g_content;
/*
 * [ // [ labels ]
 *  [ // [ pairs ]
 *   [ // [ 2 ] - key : value
 *    "key",
 *    "value"
 *   ]
 *  ]
 * ]
 */

static void addLabel( const char* _text ) {
    const size_t l_textLength = strlen( _text );

    g_labelCount++;
    const size_t l_labelIndex = ( g_labelCount - 1 );

    g_labels = ( char** )realloc( g_labels, g_labelCount * sizeof( char* ) );

    g_labels[ l_labelIndex ] =
        ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    memcpy( g_labels[ l_labelIndex ], _text, ( l_textLength + 1 ) );

    g_content =
        ( char**** )realloc( g_content, g_labelCount * sizeof( char*** ) );

    g_content[ l_labelIndex ] = ( char*** )malloc( 1 * sizeof( char** ) );
    g_content[ l_labelIndex ][ 0 ] = ( char** )malloc( 1 * sizeof( char* ) );

    ( *( size_t* )( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) ) = 1;
}

static void addContent( const char* _text, const content_t _contentType ) {
    const size_t l_textLength = strlen( _text );

    const size_t l_labelIndex = ( g_labelCount - 1 );
    const size_t l_keysCount =
        ( size_t )( g_content[ l_labelIndex ][ 0 ][ 0 ] );

    const size_t l_keyIndex = ( l_keysCount - 1 );

    char** l_content =
        &( g_content[ l_labelIndex ][ l_keyIndex ][ _contentType ] );

    *l_content = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    memcpy( *l_content, _text, ( l_textLength + 1 ) );
}

static void addKey( const char* _text ) {
    const size_t l_labelIndex = ( g_labelCount - 1 );

    ( *( size_t* )( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) )++;

    const size_t l_keysCount =
        ( size_t )( g_content[ l_labelIndex ][ 0 ][ 0 ] );
    const size_t l_keyIndex = ( l_keysCount - 1 );

    g_content[ l_labelIndex ] = ( char*** )realloc(
        g_content[ l_labelIndex ], l_keysCount * sizeof( char** ) );
    g_content[ l_labelIndex ][ l_keyIndex ] =
        ( char** )malloc( 2 * sizeof( char* ) );

    addContent( _text, KEY );
}

static void addValue( const char* _text ) {
    addContent( _text, VALUE );
}

uint16_t freeSettingsTable( void ) {
    uint16_t l_returnValue = 1;

    for ( size_t _labelIndex = 0; _labelIndex < g_labelCount; _labelIndex++ ) {
        size_t l_keysCount = ( size_t )( g_content[ _labelIndex ][ 0 ][ 0 ] );

        free( g_labels[ _labelIndex ] );

        for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
            free( g_content[ _labelIndex ][ _keyIndex ][ 0 ] );

            free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

            free( g_content[ _labelIndex ][ _keyIndex ] );
        }

        free( g_content[ _labelIndex ][ 0 ] );
        free( g_content[ _labelIndex ] );
    }

    if ( g_labelCount ) {
        free( g_labels );
        free( g_content );

        g_labelCount = 0;
    }

    if ( ( !g_labels ) && ( !g_content ) && ( !g_labelCount ) ) {
        l_returnValue = 0;
    }

    return ( l_returnValue );
}

static char* trim( const char* _text ) {
    const size_t l_textLength = strlen( _text );

    char* l_buffer = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    size_t l_bufferLength = 0;

    for ( size_t _index = 0; _index < l_textLength; _index++ ) {
        const char l_symbol = _text[ _index ];

        if ( isspace( l_symbol ) ) {
            continue;

        } else if ( l_symbol == '#' ) {
            break;
        }

        l_buffer[ l_bufferLength ] = l_symbol;
        l_bufferLength++;
    }

    l_buffer[ l_bufferLength ] = '\0';
    l_bufferLength++;

    char* l_text = ( char* )malloc( l_bufferLength * sizeof( char ) );
    memcpy( l_text, l_buffer, l_bufferLength );

    free( l_buffer );

    return ( l_text );
}

static void parseLine( char* _text ) {
    size_t l_textLength = strlen( _text );

    if ( _text[ 0 ] == '[' ) {
        _text[ l_textLength - 1 ] = '\0';

        addLabel( _text + 1 );

    } else {
        for ( size_t _index = 0; _index < ( l_textLength - 1 ); _index++ ) {
            if ( _text[ _index ] == '=' ) {
                _text[ _index ] = '\0';

                addKey( _text );
                addValue( _text + _index + 1 );

                break;
            }
        }
    }
}

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        char l_line[ MAX_LINE_LENGTH ];

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            char* l_trimmedLine = trim( l_line );
            const size_t l_lineLength = strlen( l_trimmedLine );

            if ( l_lineLength ) {
                parseLine( l_trimmedLine );
            }

            free( l_trimmedLine );
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

uint16_t readSettingsFromString( const char* _text ) {
    uint16_t l_returnValue = 1;

    size_t l_textLength = strlen( _text );
    char* l_text = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    memcpy( l_text, _text, ( l_textLength + 1 ) );

    const char l_delimiter[] = "\n";
    char* l_line = strtok( l_text, l_delimiter );

    while ( l_line ) {
        char* l_trimmedLine = trim( l_line );
        const size_t l_lineLength = strlen( l_trimmedLine );

        if ( l_lineLength ) {
            parseLine( l_trimmedLine );
        }

        free( l_trimmedLine );

        l_line = strtok( NULL, l_delimiter );
    }

    free( l_text );

    l_returnValue = 0;

    return ( l_returnValue );
}

uint16_t writeSettingsToFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        for ( size_t _labelIndex = 0; _labelIndex < g_labelCount;
              _labelIndex++ ) {
            const char* l_label = g_labels[ _labelIndex ];
            const size_t l_labelLength = strlen( l_label );
            const bool l_isNotFirstLabel = ( _labelIndex );
            const size_t l_labelBufferLength =
                ( l_labelLength + 3 + ( l_isNotFirstLabel ) );
            char* l_buffer = ( char* )malloc( l_labelBufferLength + 1 );

            if ( l_isNotFirstLabel ) {
                l_buffer[ 0 ] = '\n';
            }

            l_buffer[ 0 + ( l_isNotFirstLabel ) ] = '[';
            memcpy( ( l_buffer + 1 + ( l_isNotFirstLabel ) ), l_label,
                    l_labelLength );
            l_buffer[ l_labelLength + 1 + ( l_isNotFirstLabel ) ] = ']';
            l_buffer[ l_labelLength + 2 + ( l_isNotFirstLabel ) ] = '\n';
            l_buffer[ l_labelBufferLength ] = '\0';

            fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_labelBufferLength,
                    l_fileHandle );

            free( l_buffer );

            size_t l_keysCount =
                ( size_t )( g_content[ _labelIndex ][ 0 ][ 0 ] );

            for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
                const char* l_key = g_content[ _labelIndex ][ _keyIndex ][ 0 ];
                const size_t l_keyLength = strlen( l_key );
                const size_t l_keyBufferLength = ( l_keyLength + 2 );
                l_buffer = ( char* )malloc( l_keyBufferLength + 1 );
                memcpy( l_buffer, l_key, l_keyLength );
                l_buffer[ l_keyLength ] = ' ';
                l_buffer[ l_keyLength + 1 ] = '=';
                l_buffer[ l_keyBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_keyBufferLength,
                        l_fileHandle );

                free( l_buffer );

                const char* l_value =
                    g_content[ _labelIndex ][ _keyIndex ][ 1 ];
                const size_t l_valueLength = strlen( l_value );
                const size_t l_valueBufferLength = ( l_valueLength + 2 );
                l_buffer = ( char* )malloc( l_valueBufferLength + 1 );
                l_buffer[ 0 ] = ' ';
                memcpy( ( l_buffer + 1 ), l_value, l_valueLength );
                l_buffer[ l_valueLength + 1 ] = '\n';
                l_buffer[ l_valueBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_valueBufferLength,
                        l_fileHandle );

                free( l_buffer );
            }
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

static size_t getLabelIndex( const char* _label ) {
    for ( size_t _index = 0; _index < g_labelCount; _index++ ) {
        if ( strcmp( _label, g_labels[ _index ] ) == 0 ) {
            return ( _index );
        }
    }

    return ( UINT32_MAX );
}

static const char* const* const* getContentByIndex( const size_t _labelIndex ) {
    return ( ( const char* const* const* )( g_content[ _labelIndex ] ) );
}

const char* const* const* getSettingsContentByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex == UINT32_MAX ) {
        return ( NULL );
    }

    return ( getContentByIndex( l_labelIndex ) );
}

static size_t getKeyIndex( const char* const* const* _content,
                           const char* _key ) {
    const size_t l_contentKeyCount = ( size_t )( _content[ 0 ][ 0 ] );

    for ( size_t _index = 1; _index < l_contentKeyCount; _index++ ) {
        const char* l_key = _content[ _index ][ 0 ];

        if ( strcmp( _key, l_key ) == 0 ) {
            return ( _index );
        }
    }

    return ( UINT32_MAX );
}

static void changeKeyByIndex( const size_t _keyIndex,
                              const size_t _labelIndex,
                              const char* _value ) {
    const size_t l_valueLength = strlen( _value );

    char* l_value = ( char* )malloc( ( l_valueLength + 1 ) * sizeof( char ) );
    memcpy( l_value, _value, ( l_valueLength + 1 ) );

    free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

    g_content[ _labelIndex ][ _keyIndex ][ 1 ] = l_value;
}

uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value ) {
    uint16_t l_returnValue = 0;

    const char* const* const* l_content = getSettingsContentByLabel( _label );

    if ( l_content == NULL ) {
        l_returnValue = 1;

        goto EXIT;
    }

    const size_t l_keyIndex = getKeyIndex( l_content, _key );

    if ( l_keyIndex == UINT32_MAX ) {
        l_returnValue = 1;

        goto EXIT;
    }

    const size_t l_labelIndex = getLabelIndex( _label );

    changeKeyByIndex( l_keyIndex, l_labelIndex, _value );

EXIT:
    return ( l_returnValue );
}
