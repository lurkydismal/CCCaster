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
static char**** g_contents;
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
    g_contents = ( char**** )createArray( sizeof( char*** ) );
}

static size_t getLabelIndex( const char* _labelName ) {
    size_t l_labelIndex = UINT32_MAX;

    FOR_ARRAY( char**, g_labels ) {
        if ( strcmp( *_element, _labelName ) == 0 ) {
            l_labelIndex =
                ( _element - arrayFirstElementPointer( g_labels ) + 1 );

            break;
        }
    }

    return ( l_labelIndex );
}

static void freeLabelByIndex( const size_t _labelIndex ) {
    size_t* l_labelsCount = ( size_t* )( &( g_labels[ 0 ] ) );
    size_t* l_contentsCount = ( size_t* )( &( g_contents[ 0 ] ) );

    if ( *l_labelsCount != *l_contentsCount ) {
        exit( 1 );
    }

    if ( !( *l_labelsCount ) || !( *l_contentsCount ) ) {
        return;
    }
    free( g_labels[ _labelIndex ] );

    {
        char*** l_content = g_contents[ _labelIndex ];

#pragma omp simd
        FOR_ARRAY( char***, l_content ) {
            free( ( *_element )[ 0 ] );

            free( ( *_element )[ 1 ] );

            free( *_element );
        }

        free( l_content );
    }

    ( *l_labelsCount )--;
    ( *l_contentsCount )--;

    if ( ( *l_labelsCount ) && ( *l_contentsCount ) ) {
        for ( size_t _index = _labelIndex; _index < *l_labelsCount; _index++ ) {
            g_labels[ _index ] = g_labels[ _index + 1 ];
            g_contents[ _index ] = g_contents[ _index + 1 ];
        }

        g_labels =
            ( char** )realloc( g_labels, *l_labelsCount * sizeof( char* ) );
        g_contents = ( char**** )realloc(
            g_contents, *l_contentsCount * sizeof( char*** ) );
    }
}

static inline size_t freeLabelByLabel( const char* _label ) {
    const size_t l_labelIndex = getLabelIndex( _label );

    if ( l_labelIndex != UINT32_MAX ) {
        freeLabelByIndex( l_labelIndex );
    }

    return ( l_labelIndex );
}

static void addLabel( const char* _text ) {
    freeLabelByLabel( _text );

    insertIntoArray( ( void*** )( &g_labels ), strdup( _text ) );

    insertIntoArray( ( void*** )( &g_contents ),
                     ( char*** )createArray( sizeof( char** ) ) );
}

static inline void changeKeyByIndex( const size_t _keyIndex,
                                     const size_t _labelIndex,
                                     const char* _value ) {
    char* l_value = strdup( _value );

    free( g_contents[ _labelIndex ][ _keyIndex ][ 1 ] );

    g_contents[ _labelIndex ][ _keyIndex ][ 1 ] = l_value;
}

static char*** getContentByIndex( const size_t _labelIndex ) {
    char*** l_returnValue = ( char*** )createArray( sizeof( char** ) );
    char*** l_content = g_contents[ _labelIndex ];
    const size_t l_contentLength = arrayLength( l_content );

    preallocateArray( ( void*** )( &l_returnValue ), l_contentLength );

#pragma omp simd
    for ( size_t _index = 1; _index < ( l_contentLength + 1 ); _index++ ) {
        char** l_pair = malloc( 2 * sizeof( char* ) );

        l_pair[ 0 ] = strdup( l_content[ _index ][ 0 ] );
        l_pair[ 1 ] = strdup( l_content[ _index ][ 1 ] );

        insertIntoArrayByIndex( ( void*** )( &l_returnValue ), _index, l_pair );
    }

    return ( l_returnValue );
}

static void addContent( const char* _text, const content_t _contentType ) {
    const size_t l_labelIndex = arrayLength( g_labels );

    static char** l_pair = NULL;

    if ( ( l_pair != NULL ) && ( l_pair[ _contentType ] != NULL ) ) {
        free( l_pair[ _contentType ] );
    }

    if ( l_pair == NULL ) {
        l_pair = ( char** )malloc( 2 * sizeof( char* ) );

        l_pair[ KEY ] = NULL;
        l_pair[ VALUE ] = NULL;
    }

    l_pair[ _contentType ] = strdup( _text );

    if ( l_pair != NULL ) {
        if ( ( l_pair[ KEY ] != NULL ) && ( l_pair[ VALUE ] != NULL ) ) {
            char**** l_content = &( g_contents[ l_labelIndex ] );

            insertIntoArray( ( void*** )( l_content ), l_pair );

            l_pair = NULL;
        }
    }
}

static void addKey( const char* _text ) {
    addContent( _text, KEY );
}

static inline void addValue( const char* _text ) {
    addContent( _text, VALUE );
}

static inline char* trim( const char* _text ) {
    const size_t l_textLength = strlen( _text );

    char* l_buffer = ( char* )malloc( ( l_textLength + 1 ) * sizeof( char ) );
    size_t l_bufferLength = 0;

    for ( const char* _symbol = _text; _symbol < ( _text + l_textLength );
          _symbol++ ) {
        if ( isspace( *_symbol ) ) {
            continue;

        } else if ( *_symbol == '#' ) {
            break;
        }

        l_buffer[ l_bufferLength ] = *_symbol;
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

inline char*** getSettingsContentByLabel( const char* _label ) {
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
    printf( "CHANGE %s %s %s\n", _key, _label, _value );

    char*** l_content = getSettingsContentByLabel( _label );

    if ( l_content == NULL ) {
        l_returnValue = 1;

        goto EXIT;
    }

    {
        const size_t l_keyIndex = findKeyInSettings( l_content, _key );

        freeSettingsContent( l_content );

        const size_t l_labelIndex = getLabelIndex( _label );

        if ( l_keyIndex == UINT32_MAX ) {
            l_returnValue = ENOKEY;

            size_t* l_labelsCount = ( size_t* )( &( g_labels[ 0 ] ) );

            const size_t l_labelCountBackup = *l_labelsCount;
            *l_labelsCount = ( l_labelIndex + 1 );

            addKey( _key );
            addValue( _value );

            *l_labelsCount = l_labelCountBackup;

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
    writeSettingsToFile( SETTINGS_BACKUP_FILE_PATH );

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

    if ( ( l_returnValue = writeSettingsToFile( SETTINGS_FILE_PATH ) ) != 0 ) {
        goto EXIT;
    }

    if ( ( l_returnValue = writeSettingsToFile( SETTINGS_BACKUP_FILE_PATH ) ) !=
         0 ) {
        goto EXIT;
    }

EXIT:
    return ( l_returnValue );
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
#pragma omp simd
        FOR_ARRAY( char**, g_labels ) {
            const size_t l_labelIndex =
                ( _element - arrayFirstElementPointer( g_labels ) );

            // Label
            {
                const bool l_isNotFirstLabel = ( l_labelIndex );

                safeConcatBeforeAndAfterStringAndWriteToFile(
                    *_element, ( ( l_isNotFirstLabel ) ? ( "\n[" ) : ( "[" ) ),
                    "]\n", l_fileHandle );
            }

            char*** l_content = g_contents[ l_labelIndex + 1 ];

            FOR_ARRAY( char***, l_content ) {
                const char* l_key = ( *_element )[ 0 ];

                safeConcatBeforeAndAfterStringAndWriteToFile( l_key, "", " =",
                                                              l_fileHandle );

                const char* l_value = ( *_element )[ 1 ];

                safeConcatBeforeAndAfterStringAndWriteToFile(
                    l_value, " ", "\n", l_fileHandle );
            }
        }

        l_returnValue = 0;

        fclose( l_fileHandle );
    }

    return ( l_returnValue );
}

uint16_t freeSettingsTable( void ) {
    uint16_t l_returnValue = 1;

    for ( size_t _labelsLength = arrayLength( g_labels ); _labelsLength > 0;
          _labelsLength-- ) {
        freeLabelByIndex( _labelsLength );
    }

    if ( g_labels ) {
        free( g_labels );
    }

    if ( g_contents ) {
        free( g_contents );
    }

    if ( ( !g_labels ) && ( !g_contents ) && ( !arrayLength( g_labels ) ) ) {
        l_returnValue = 0;
    }

    return ( l_returnValue );
}

void printSettings( void ) {
    const size_t l_labelsLength = arrayLength( g_labels );

    for ( size_t _labelIndex = 0; _labelIndex < l_labelsLength;
          _labelIndex++ ) {
        const char* l_label = g_labels[ _labelIndex + 1 ];
        char*** l_content = g_contents[ _labelIndex + 1 ];

        printf( "[%s] : %u items\n", l_label, arrayLength( l_content ) );

        FOR_ARRAY( char***, l_content ) {
            const char* l_key = ( *_element )[ 0 ];
            const char* l_value = ( *_element )[ 1 ];

            printf( "\"%s\" = \"%s\"\n", l_key, l_value );
        }
    }
}
