#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 255

typedef enum {
    KEY,
    VALUE
} content_t;

char** g_labels;
size_t g_labelCount = 0;
char**** g_content;
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

static void freeTable( void );
static bool readFromFile( const char* _fileName );
static void writeToFile( const char* _fileName );

static void addLabel( const char* _text, const size_t _textLength ) {
    g_labelCount++;
    const size_t l_labelIndex = ( g_labelCount - 1 );

    g_labels = (char**)realloc( g_labels, g_labelCount * sizeof( char* ) );

    g_labels[ l_labelIndex ] = (char*)malloc( ( _textLength + 1 ) * sizeof( char ) );
    memcpy( g_labels[ l_labelIndex ], _text, _textLength );
    g_labels[ l_labelIndex ][ _textLength ] = '\0';

    g_content = (char****)realloc( g_content, g_labelCount * sizeof( char*** ) );

    g_content[ l_labelIndex ] = (char***)malloc( 1 * sizeof( char** ) );
    g_content[ l_labelIndex ][ 0 ] = (char**)malloc( 1 * sizeof( char* ) );

    ( *(size_t*)( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) ) = 1;
}

static void addContent( const char* _text, const size_t _textLength, const content_t _contentType ) {
    const size_t l_labelIndex = ( g_labelCount - 1 );
    const size_t l_keysCount = (size_t)( g_content[ l_labelIndex ][ 0 ][ 0 ] );

    const size_t l_keyIndex = ( l_keysCount - 1 );

    char** l_content = &( g_content[ l_labelIndex ][ l_keyIndex ][ _contentType ] );

    const bool l_isNotNUlTerminated = ( _text[ _textLength - 1 ] != '\0' );

    *l_content = (char*)malloc( ( _textLength + (size_t)l_isNotNUlTerminated ) * sizeof( char ) );
    memcpy( *l_content, _text, _textLength );

    if ( l_isNotNUlTerminated ) {
        ( *l_content )[ _textLength ] = '\0';
    }
}

static void addKey( const char* _text, const size_t _textLength ) {
    const size_t l_labelIndex = ( g_labelCount - 1 );

    ( *(size_t*)( &( g_content[ l_labelIndex ][ 0 ][ 0 ] ) ) )++;

    const size_t l_keysCount = (size_t)( g_content[ l_labelIndex ][ 0 ][ 0 ] );
    const size_t l_keyIndex = ( l_keysCount - 1 );

    g_content[ l_labelIndex ] = (char***)realloc( g_content[ l_labelIndex ], l_keysCount * sizeof( char** ) );
    g_content[ l_labelIndex ][ l_keyIndex ] = (char**)malloc( 2 * sizeof( char* ) );

    addContent( _text, _textLength, KEY );
}

static void addValue( const char* _text, const size_t _textLength ) {
    addContent( _text, _textLength, VALUE );
}

int main( void ) {
#if 0
    {
        const char l_text[] = "label";

        addLabel( l_text, sizeof( l_text ) );

        const char l_key[] = "kkey";

        addKey( l_key, sizeof( l_key ) );

        const char l_value[] = { 'v', 'v', 'v', 'a', 'l', 'u', 'e' };

        addValue( l_value, sizeof( l_value ) );
    }
#endif

    if ( readFromFile( "t.ini" ) ) {
        writeToFile( "tc.ini" );
    }

    freeTable();

    return (0);
}

static void freeTable( void ) {
    for ( size_t _labelIndex = 0; _labelIndex < g_labelCount; _labelIndex++ ) {
        size_t l_keysCount = (size_t)( g_content[ _labelIndex ][ 0 ][ 0 ] );

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
}

static char* trim( const char* _text, const size_t _textLength ) {
    size_t l_textLength = 0;
    char* l_buffer = (char*)malloc( ( _textLength + 1 ) * sizeof( char ) );

    for ( size_t _index = 0; _index < _textLength; _index++ ) {
        const char l_symbol = _text[ _index ];

        if ( isspace( l_symbol ) ) {
            continue;

        } else if ( l_symbol == '#' ) {
            break;
        }

        l_buffer[ l_textLength ] = l_symbol;
        l_textLength++;
    }

    char* l_text = (char*)malloc( ( l_textLength + 1 ) * sizeof( char ) );
    memcpy( l_text, l_buffer, l_textLength );
    l_text[ l_textLength ] = '\0';

    free( l_buffer );

    return ( l_text );
}

static void parseLine( char* _text, size_t _textLength ) {
    if ( _text[ 0 ] == '[' ) {
        _text++;
        _textLength -= 2;
        _text[ _textLength ] = '\0';

        addLabel( _text, _textLength );

    } else {
        for ( size_t _index = 0; _index < _textLength; _index++ ) {
            if ( _text[ _index ] == '=' ) {
                addKey( _text, _index );
                addValue( ( _text + _index + 1 ), ( _textLength - _index ) );

                break;
            }
        }
    }
}

static bool readFromFile( const char* _fileName ) {
    bool l_returnValue = false;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        char l_line[ MAX_LINE_LENGTH ];

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            char* l_trimmedLine = trim( l_line, strlen( l_line ) );
            const size_t l_lineLength = strlen( l_trimmedLine );

            if ( l_lineLength ) {
                parseLine( l_trimmedLine, l_lineLength );
            }

            free( l_trimmedLine );
        }

        l_returnValue = true;
    }

    fclose( l_fileHandle );

    return ( l_returnValue );
}

static void writeToFile( const char* _fileName ) {
    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        for ( size_t _labelIndex = 0; _labelIndex < g_labelCount; _labelIndex++ ) {
            const char* l_label = g_labels[ _labelIndex ];
            const size_t l_labelLength = strlen( l_label );
            const bool l_isNotFirstLabel = ( _labelIndex );
            const size_t l_labelBufferLength = ( l_labelLength + 3 + ( l_isNotFirstLabel ) );
            char* l_buffer = (char*)malloc( l_labelBufferLength + 1 );

            if ( l_isNotFirstLabel ) {
                l_buffer[ 0 ] = '\n';
            }

            l_buffer[ 0 + ( l_isNotFirstLabel ) ] = '[';
            memcpy( ( l_buffer + 1 + ( l_isNotFirstLabel ) ), l_label, l_labelLength );
            l_buffer[ l_labelLength + 1 + ( l_isNotFirstLabel ) ] = ']';
            l_buffer[ l_labelLength + 2 + ( l_isNotFirstLabel ) ] = '\n';
            l_buffer[ l_labelBufferLength ] = '\0';

            fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_labelBufferLength, l_fileHandle );

            free( l_buffer );

            size_t l_keysCount = (size_t)( g_content[ _labelIndex ][ 0 ][ 0 ] );

            for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
                const char* l_key = g_content[ _labelIndex ][ _keyIndex ][ 0 ];
                const size_t l_keyLength = strlen( l_key );
                const size_t l_keyBufferLength = ( l_keyLength + 2 );
                l_buffer = (char*)malloc( l_keyBufferLength + 1 );
                memcpy( l_buffer, l_key, l_keyLength );
                l_buffer[ l_keyLength ] = ' ';
                l_buffer[ l_keyLength + 1 ] = '=';
                l_buffer[ l_keyBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_keyBufferLength, l_fileHandle );

                free( l_buffer );

                const char* l_value = g_content[ _labelIndex ][ _keyIndex ][ 1 ];
                const size_t l_valueLength = strlen( l_value );
                const size_t l_valueBufferLength = ( l_valueLength + 2 );
                l_buffer = (char*)malloc( l_valueBufferLength + 1 );
                l_buffer[ 0 ] = ' ';
                memcpy( ( l_buffer + 1 ) , l_value, l_valueLength );
                l_buffer[ l_valueLength + 1 ] = '\n';
                l_buffer[ l_valueBufferLength ] = '\0';

                fwrite( l_buffer, sizeof( l_buffer[ 0 ] ), l_valueBufferLength, l_fileHandle );

                free( l_buffer );
            }
        }
    }

    fclose( l_fileHandle );
}
