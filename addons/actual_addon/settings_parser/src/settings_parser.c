#include "settings_parser.h"

#include <ctype.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdfunc.h"

#define ENOKEY 126

typedef enum { KEY, VALUE } content_t;

static char** g_labels;
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

void settingsParserInitialize( void ) {
    g_labels = ( char** )createArray( sizeof( char* ) );
    g_content = ( char**** )createArray( sizeof( char*** ) );
}

static size_t getLabelIndex( const char* _label ) {
    const size_t l_labelCount = arrayLength( g_labels );
    size_t l_labelIndex = UINT32_MAX;

    for ( size_t _index = 0; _index < l_labelCount; _index++ ) {
        if ( strcmp( _label, g_labels[ _index ] ) == 0 ) {
            l_labelIndex = _index;

            break;
        }
    }

    return ( l_labelIndex );
}

static void freeLabelByIndex( const size_t _labelIndex ) {
    size_t* l_labelCount = (size_t*)( &( g_labels[ 0 ] ) );

    if ( !( *l_labelCount ) ) {
        return;
    }

    free( g_labels[ _labelIndex ] );

    {
        char*** l_content = g_content[ _labelIndex ];
        const size_t l_contentLength = arrayLength( l_content );
        char*** l_contentFirstElement = arrayFirstElementPointer( l_content );
        char** const* l_contentEnd = ( l_contentFirstElement + l_contentLength );

#pragma omp simd
        for ( char*** _pair = l_contentFirstElement; _pair != l_contentEnd; _pair++ ) {
            free( ( *_pair )[ 0 ] );

            free( ( *_pair )[ 1 ] );

            free( *_pair );
        }

        free( l_content[ 0 ] );
        free( l_content );
    }

    ( *l_labelCount )--;

    if ( *l_labelCount ) {
        for ( size_t _index = _labelIndex; _index < *l_labelCount; _index++ ) {
            g_labels[ _index ] = g_labels[ _index + 1 ];
            g_content[ _index ] = g_content[ _index + 1 ];
        }

        g_labels =
            ( char** )realloc( g_labels, *l_labelCount * sizeof( char* ) );
        g_content =
            ( char**** )realloc( g_content, *l_labelCount * sizeof( char*** ) );
    }

    // TODO: Free g_labels and g_content on empty
}

static /* inline */ size_t freeLabelByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex != UINT32_MAX ) {
        freeLabelByIndex( l_labelIndex );
    }

    return ( l_labelIndex );
}

static void addLabel( const char* _text ) {
    freeLabelByLabel( _text );

    insertIntoArray( (void***)( &g_labels ), strdup( _text ) );

    insertIntoArray( (void***)( &g_content ),( char*** )createArray( sizeof( char** ) ) );
}

static /* inline */ void changeKeyByIndex( const size_t _keyIndex,
                                           const size_t _labelIndex,
                                           const char* _value ) {
    char* l_value = strdup( _value );

    free( g_content[ _labelIndex ][ _keyIndex ][ 1 ] );

    g_content[ _labelIndex ][ _keyIndex ][ 1 ] = l_value;
}

static char*** getContentByIndex( const size_t _labelIndex ) {
    char*** l_content = g_content[ _labelIndex ];
    const size_t l_contentLength = arrayLength( l_content[ 0 ] );

    char*** l_returnValue =
        ( char*** )createArray( sizeof( char** ) );

    preallocateArray( (void***)( &l_returnValue ), l_contentLength );

#pragma omp simd
    for ( size_t _index = 1; _index < l_contentLength; _index++ ) {
        char** l_pair = ( char** )malloc( 2 * sizeof( char* ) );

        l_pair[ 0 ] = strdup( l_content[ _index ][ 0 ] );
        l_pair[ 1 ] = strdup( l_content[ _index ][ 1 ] );

        insertIntoArrayByIndex( (void***)( &l_returnValue ), _index, l_pair );
    }

    return ( l_returnValue );
}

static void addContent( const char* _text, const content_t _contentType ) {
    const size_t l_labelIndex = arrayLength( g_labels );
    printf( "labels %li\n", arrayLength( g_labels ) );
    printf( "contents %li\n", arrayLength( g_content ) );
    printf( "contents[] %li\n", arrayLength( g_content[ l_labelIndex ] ) );

    static char** l_pair = NULL;

    if ( ( l_pair != NULL ) && ( l_pair[ _contentType ] != NULL ) ) {
        free( l_pair[ _contentType ] );
    }

    if ( l_pair == NULL ) {
        l_pair =
            ( char** )malloc( 2 * sizeof( char* ) );

        l_pair[ KEY ] = NULL;
        l_pair[ VALUE ] = NULL;
    }

    if ( _contentType == KEY ) {
        printf( "key \'%s\'\n", _text );

    } else if ( _contentType == VALUE ) {
        printf( "value \'%s\'\n", _text );
    }

    l_pair[ _contentType ] = strdup( _text );

    if ( l_pair != NULL ) {
        if ( ( l_pair[ KEY ] != NULL ) && ( l_pair[ VALUE ] != NULL ) ) {
            printf( "insert\n" );
            char*** l_content = g_content[ l_labelIndex ];

            insertIntoArray( (void***)( &l_content ), l_pair );
            printf( "%d\n", arrayLength( l_content ) );

            l_pair = NULL;
        }
    }
}

static size_t getKeyIndex( char*** _content, const char* _key ) {
    size_t l_keyIndex = UINT32_MAX;
    char*** l_contentFirstElement = arrayFirstElementPointer( _content );
    const size_t l_contentLength = arrayLength( _content[ 0 ] );
    char** const* l_contentEnd = ( l_contentFirstElement + l_contentLength );

    for ( char*** _pair = l_contentFirstElement; _pair != l_contentEnd; _pair++ ) {
        const char* l_key = ( *_pair )[ 0 ];

        if ( strcmp( _key, l_key ) == 0 ) {
            l_keyIndex = ( _pair - l_contentFirstElement );

            break;
        }
    }

    return ( l_keyIndex );
}

static void addKey( const char* _text ) {
    addContent( _text, KEY );
}

static inline void addValue( const char* _text ) {
    addContent( _text, VALUE );
}

static /* inline */ char* trim( const char* _text ) {
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

    l_buffer = ( char* )realloc( l_buffer, l_bufferLength );

    return ( l_buffer );
}

static void parseLine( char* _text ) {
    char* l_trimmedText = trim( _text );
    const size_t l_textLength = strlen( l_trimmedText );

    if ( !l_textLength ) {
        goto EXIT;
    }

    if ( l_trimmedText[ 0 ] == '[' ) {
        l_trimmedText[ l_textLength - 1 ] = '\0';
        char* l_label = ( l_trimmedText + 1 );

        addLabel( l_label );

    } else {
        for ( size_t _index = 0; _index < ( l_textLength - 1 ); _index++ ) {
            if ( l_trimmedText[ _index ] == '=' ) {
                l_trimmedText[ _index ] = '\0';

                const char* l_key = l_trimmedText;
                const char* l_value = ( l_trimmedText + _index + 1 );

                addKey( l_key );
                addValue( l_value );

                break;
            }
        }
    }

EXIT:
    free( l_trimmedText );
}

/* inline */ char*** getSettingsContentByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex == UINT32_MAX ) {
        return ( NULL );
    }

    return ( getContentByIndex( l_labelIndex ) );
}

uint16_t changeSettingsKeyByLabel( const char* _key,
                                   const char* _label,
                                   const char* _value ) {
    uint16_t l_returnValue = 0;

    char*** l_content = getSettingsContentByLabel( _label );

    if ( l_content == NULL ) {
        l_returnValue = 1;

        goto EXIT;
    }

    {
        const size_t l_keyIndex = getKeyIndex( l_content, _key );

        size_t l_keysCount = arrayLength( l_content[ 0 ] );

#pragma omp simd
        for ( size_t _keyIndex = 1; _keyIndex < l_keysCount; _keyIndex++ ) {
            char** l_pair = l_content[ _keyIndex ];

            free( l_pair[ 0 ] );

            free( l_pair[ 1 ] );

            free( l_pair );
        }

        free( l_content[ 0 ] );
        free( l_content );

        const size_t l_labelIndex = getLabelIndex( _label );

        if ( l_keyIndex == UINT32_MAX ) {
            l_returnValue = ENOKEY;

            size_t* l_labelCount = (size_t*)( &( g_labels[ 0 ] ) );

            const size_t l_labelCountBackup = *l_labelCount;
            *l_labelCount = ( l_labelIndex + 1 );

            addKey( _key );
            addValue( _value );

            *l_labelCount = l_labelCountBackup;

            goto WRITE;
        }

        {
            // Label does exist and key does exist
            // Change key in label
            changeKeyByIndex( l_keyIndex, l_labelIndex, _value );
        }
    }
WRITE:
    writeSettingsToFile( SETTINGS_FILE_PATH );

EXIT:
    return ( l_returnValue );
}

uint16_t readSettingsFromFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "r" );

    if ( l_fileHandle ) {
        char l_line[ MAX_LINE_LENGTH ];

        while ( fgets( l_line, MAX_LINE_LENGTH, l_fileHandle ) != NULL ) {
            parseLine( l_line );
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

uint16_t readSettingsFromString( const char* _text ) {
    uint16_t l_returnValue = 1;

    char* l_text = strdup( _text );

    const char l_delimiter[] = "\n";
    char* l_line = strtok( l_text, l_delimiter );

    while ( l_line ) {
        parseLine( l_line );

        l_line = strtok( NULL, l_delimiter );
    }

    free( l_text );

    l_returnValue = writeSettingsToFile( SETTINGS_BACKUP_FILE_PATH );

    return ( l_returnValue );
}

static size_t concatBeforeAndAfterString( char** _string,
                                          const char* _beforeString,
                                          const char* _afterString ) {
    const size_t l_stringLength = strlen( *_string );
    const size_t l_beforeStringLength = strlen( _beforeString );
    const size_t l_afterStringLegnth = strlen( _afterString );
    const size_t l_totalLength =
        ( l_beforeStringLength + l_stringLength + l_afterStringLegnth );

    char* l_buffer = ( char* )malloc( l_stringLength * sizeof( char ) );

    memcpy( l_buffer, *_string, l_stringLength );

    *_string =
        ( char* )realloc( *_string, ( l_totalLength + 1 ) * sizeof( char ) );

    memcpy( *_string, _beforeString, l_beforeStringLength );
    memcpy( ( l_beforeStringLength + *_string ), l_buffer, l_stringLength );
    memcpy( ( l_beforeStringLength + l_stringLength + *_string ), _afterString,
            l_afterStringLegnth );

    ( *_string )[ l_totalLength ] = '\0';

    free( l_buffer );

    return ( l_totalLength );
}

static bool concatBeforeAndAfterStringAndWriteToFile( char** _string,
                                                      const char* _beforeString,
                                                      const char* _afterString,
                                                      FILE* _fileHandle ) {
    const size_t l_stringLength =
        concatBeforeAndAfterString( _string, _beforeString, _afterString );

    return ( fwrite( *_string, sizeof( ( *_string )[ 0 ] ), l_stringLength,
                     _fileHandle ) == l_stringLength );
}

static bool safeConcatBeforeAndAfterStringAndWriteToFile(
    const char* _string,
    const char* _beforeString,
    const char* _afterString,
    FILE* _fileHandle ) {
    bool l_returnValue = false;

    char* l_string = strdup( _string );

    l_returnValue = concatBeforeAndAfterStringAndWriteToFile(
                    &l_string, _beforeString, _afterString, _fileHandle );

    free( l_string );

    return ( l_returnValue );
}

uint16_t writeSettingsToFile( const char* _fileName ) {
    uint16_t l_returnValue = 1;

    FILE* l_fileHandle = fopen( _fileName, "w" );

    if ( l_fileHandle ) {
        const size_t l_labelCount = arrayLength( g_labels );

#pragma omp simd
        for ( size_t _labelIndex = 0; _labelIndex < l_labelCount;
              _labelIndex++ ) {
            // Label
            {
                const char* l_label = g_labels[ _labelIndex ];
                const bool l_isNotFirstLabel = ( _labelIndex );

                safeConcatBeforeAndAfterStringAndWriteToFile(
                    l_label, ( ( l_isNotFirstLabel ) ? ( "\n[" ) : ( "[" ) ),
                    "]\n", l_fileHandle );
            }

            char*** l_content = g_content[ _labelIndex ];
            size_t l_contentLength = arrayLength( l_content[ 0 ] );
            char*** l_contentFirstElement =
                arrayFirstElementPointer( l_content );
            char** const* l_contentEnd =
                ( l_contentFirstElement + l_contentLength );

            for ( char*** _pair = l_contentFirstElement;
                  _pair != l_contentEnd; _pair++ ) {
                const char* l_key = ( *_pair )[ 0 ];

                safeConcatBeforeAndAfterStringAndWriteToFile( l_key, "", " =",
                                                              l_fileHandle );

                const char* l_value = ( *_pair )[ 1 ];

                safeConcatBeforeAndAfterStringAndWriteToFile(
                    l_value, " ", "\n", l_fileHandle );
            }
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

/* inline */ uint16_t freeSettingsTable( void ) {
    uint16_t l_returnValue = 1;

    for ( size_t _labelCount = arrayLength( g_labels ); _labelCount > 0;
          _labelCount-- ) {
        freeLabelByIndex( _labelCount - 1 );
    }

    if ( g_labels ) {
        free( g_labels );
    }

    if ( g_content ) {
        free( g_content );
    }

    if ( ( !g_labels ) && ( !g_content ) && ( !arrayLength( g_labels ) ) ) {
        l_returnValue = 0;
    }

    return ( l_returnValue );
}

void printSettings( void ) {
    const size_t l_labelCount = arrayLength( g_labels );

    for ( size_t _labelIndex = 0; _labelIndex < l_labelCount; _labelIndex++ ) {
        const char* l_label = g_labels[ _labelIndex ];
        char*** l_content = g_content[ _labelIndex ];
        const size_t l_contentLength = arrayLength( l_content[ 0 ] );
        char*** l_contentFirstElement = arrayFirstElementPointer( l_content );
        char** const* l_contentEnd =
            ( l_contentFirstElement + l_contentLength );

        printf( "[%s] : %d %p\n", l_label, l_contentLength, l_contentEnd );

        for ( char*** _elementPointer = l_contentFirstElement;
              _elementPointer != l_contentEnd; _elementPointer++ ) {
            const char* l_key = ( *_elementPointer )[ 0 ];
            const char* l_value = ( *_elementPointer )[ 1 ];

            printf( "%s=%s\n", l_key, l_value );
        }
    }
}
