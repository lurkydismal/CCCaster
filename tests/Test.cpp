#ifndef RELEASE

#include <gtest/gtest.h>

using namespace std;

int RunAllTests( int& argc, char* argv[] ) {
    testing::InitGoogleTest( &argc, argv );
    return RUN_ALL_TESTS();
}

#endif // NOT RELEASE
