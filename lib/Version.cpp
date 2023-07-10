#include "Version.hpp"

#include <vector>

using namespace std;

// List of versions that made compatibility breaking changes between the same
// major.minor versions. There is also an implicit assumption that compatibility
// is broken whenever the minor version increments.
static vector< Version > breakingVersions = {
    "2.1e",     // Changed protocol by adding UdpControl::Disconnect
    "3.0a.019", // Changed round over logic
};

string Version::get( PartEnum part ) const {
    size_t i = code.find( '.' );

    if ( part == Major )
        return code.substr( 0, i );

    if ( i == string::npos )
        return "";

    size_t j = code.find_first_not_of( "0123456789", i + 1 );

    if ( part == Minor )
        return code.substr( i + 1, j - ( i + 1 ) );

    if ( j == string::npos )
        return "";

    if ( code[ j ] == '.' )
        ++j;

    return code.substr( j );
}

bool Version::isCustom() const {
    if ( revision.empty() )
        return false;

    return ( revision.rfind( "-custom" ) + 7 == revision.size() );
}

bool Version::isSimilar( const Version& other, uint8_t level ) const {
    switch ( level ) {
        default:
        case 4:
            if ( buildTime != other.buildTime )
                return false;

        case 3:
            if ( isCustom() && other.isCustom() )
                return ( buildTime == other.buildTime );

            if ( revision != other.revision )
                return false;

        case 2:
            if ( suffix() != other.suffix() )
                return false;

        case 1:
            if ( level == 1 ) {
                for ( const Version& divider : breakingVersions ) {
                    if ( *this >= divider && other < divider )
                        return false;

                    if ( *this < divider && other >= divider )
                        return false;
                }
            }

            if ( minor() != other.minor() )
                return false;

        case 0:
            if ( major() != other.major() )
                return false;
            break;
    }

    return true;
}

bool operator<( const Version& a, const Version& b ) {
    if ( a.major() < b.major() )
        return true;

    if ( a.major() > b.major() )
        return false;

    if ( a.minor() < b.minor() )
        return true;

    if ( a.minor() > b.minor() )
        return false;

    if ( a.suffix() < b.suffix() )
        return true;

    if ( a.suffix() > b.suffix() )
        return false;

    return false;
}

#include "Version.local.hpp"
