#include <stdio.h>
#include <stdlib.h>

#include "settings_parser.h"

int main( void ) {
    settingsParserInitialize();

    if ( readSettingsFromFile( "t.ini" ) == 0 ) {
        printf( "1\n" );

        printSettings();

        printf( "\"zaxc\", \"wqetrcw\", \"test\" %s\n",
                ( changeSettingsKeyByLabel( "zaxc", "wqetrcw", "test" ) == 1 )
                    ? "Label not found"
                    : "wrong result" );

        char*** l_settings = getSettingsContentByLabel( "TestLabel" );

        printf( "Settings qwe: %s\n", l_settings[ 2 ][ 1 ] );

        printf(
            "\"qwe\", \"TestLabel\", \"testtt\" %s\n",
            ( changeSettingsKeyByLabel( "qwe", "TestLabel", "testtt" ) == 126 )
                ? "Key not found"
                : "wrong result" );

        printf( "Settings qwe: %s\n", l_settings[ 2 ][ 1 ] );

        {
            char*** l_content = l_settings;
            const size_t l_contentLength = ( ( size_t )( l_content[ 0 ] ) - 1 );
            char*** l_contentFirstElement = ( l_content + 1 );
            char** const* l_contentEnd =
                ( l_contentFirstElement + l_contentLength );

#pragma omp simd
            for ( char*** _pair = l_contentFirstElement; _pair != l_contentEnd;
                  _pair++ ) {
                free( ( *_pair )[ 0 ] );

                free( ( *_pair )[ 1 ] );

                free( ( *_pair ) );
            }

            free( l_content );
        }

        writeSettingsToFile( "tc.ini" );
    }

    printf( "2\n" );

    const char l_defaultSettings[] =
        "[keyboard]\n"
        "38 = 8\n"
        "39 = 6\n"
        "40 = 2\n"
        "37 = 4\n"
        "90 = A\n"
        "88 = B\n"
        "67 = C\n"
        "86 = D\n"
        "68 = E\n"
        "83 = AB\n"
        "221 = FN1\n"
        "82 = FN2\n"
        "84 = START\n"
        "115 = ToggleOverlay_KeyConfig_Native\n";

    readSettingsFromString( l_defaultSettings );

    printSettings();

    printf( "3\n" );

    const char l_defaultSettings2[] =
        "[keyboard]\n"
        "3 = AB\n"
        "21 = FN1\n"
        "2 = FN2\n"
        "4 = START\n";

    readSettingsFromString( l_defaultSettings2 );

    printSettings();

    printf( "4\n" );

    printf( "\"zaxc\", \"wqetrcw\", \"test\" %s\n",
            ( changeSettingsKeyByLabel( "zaxc", "wqetrcw", "test" ) == 1 )
                ? "Label not found"
                : "wrong result" );
    printf( "\"82\", \"keyboard\", \"fn3\" %s\n",
            ( changeSettingsKeyByLabel( "82", "keyboard", "fn3" ) == 126 )
                ? "Key not found"
                : "Wrong result" );
    printf( "\"81\", \"keyboard\", \"fn4\" %s\n",
            ( changeSettingsKeyByLabel( "81", "keyboard", "fn4" ) == 126 )
                ? "Key not found"
                : "Wrong result" );

    writeSettingsToFile( "tca.ini" );

    freeSettingsTable();

    return ( 0 );
}
