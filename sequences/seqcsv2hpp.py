import sys

_, cmoon, hmoon, fmoon = sys.argv

def writeSequence( filename ):
    charaname = filename[:-4]
    moon
    print( charaname )

header = \
"""#pragma once

#include <map>

"""
with open( "test.hpp" , 'w' ) as f:
    f.writelines( header )
    writeSequence( cmoon )
    writeSequence( hmoon )
    writeSequence( fmoon )
