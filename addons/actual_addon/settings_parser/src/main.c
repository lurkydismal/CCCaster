#include <stdio.h>

#include "settings_parser.h"

int main( void ) {
    if ( readSettingsFromFile( "t.ini" ) == 0 ) {
        printf( "1\n" );

        printSettings();

        changeSettingsKeyByLabel( "zaxc", "wqetrcw", "test" );
        changeSettingsKeyByLabel( "qwe", "TestLabel", "testtt" );

        writeSettingsToFile( "tc.ini" );

    } else {
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

        printf( "%d\n", changeSettingsKeyByLabel( "zaxc", "wqetrcw", "test" ) );
        printf( "%d\n", changeSettingsKeyByLabel( "82", "keyboard", "fn3" ) );
        printf( "%d\n", changeSettingsKeyByLabel( "81", "keyboard", "fn4" ) );

        writeSettingsToFile( "tc.ini" );
    }

    freeSettingsTable();

    return ( 0 );
}
