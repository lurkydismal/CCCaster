#include "json_file.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "nlohmann/json.hpp"
// #include <fmt/core.h>
// #include <icecream/icecream.hpp>
#include <fstream>

using json = nlohmann::json;

JSONFile::JSONFile( void ) {
    this->_fileName = "";
    this->_fileExtension = "";
    this->_filePath = "";
}

JSONFile::JSONFile( std::string const& _fileName,
                    std::string const& _fileExtension ) {
    this->_fileName = _fileName;
    this->_fileExtension = _fileExtension;
    // this->_filePath =
    // fmt::format(
    //     "{}.{}", this->_filePath,
    //     this->_fileExtension );

    this->_filePath = this->_fileName + "." + this->_fileExtension;

    // IC( this->_filePath );

    // IC( this->_json.empty() );

    // Parse JSON file
            MessageBoxA( 0, "test3", "test", 0 );
    try {
        ;
            MessageBoxA( 0, ( this->_filePath ).c_str(), "test", 0 );
        std::ifstream l_jsonFileIn( this->_filePath );
            MessageBoxA( 0, ( this->_filePath ).c_str(), "test", 0 );

        // Is controls preferences file found
        // IC( this->_json.good() );

        if ( l_jsonFileIn.good() ) {
            MessageBoxA( 0, "test", "test", 0 );
            this->_json = json::parse( l_jsonFileIn );

        } else {
            MessageBoxA( 0, "test2", "test", 0 );
            // const std::string
            // l_preferencesBackupFilePath =
            // fmt::format(
            //     "{}_backup.{}",
            //     this->_fileName,
            //     this->_fileExtension );

            const std::string l_preferencesBackupFilePath =
                this->_fileName + "_backup." + this->_fileExtension;

            std::ifstream l_jsonBackupFileIn( l_preferencesBackupFilePath );

            this->_json = json::parse( l_jsonBackupFileIn );
        }
    } catch (std::exception& e) {
        MessageBoxA( 0, "fail", "test", 0 );
    }

    // IC( this->_json.dump( 4 ) );
}

uint16_t JSONFile::overwrite( void ) {
    uint16_t l_returnValue = 0;
    std::ofstream l_jsonFileOut;

    // IC( this->_filePath );

    l_jsonFileOut.open( this->_filePath );

    if ( l_jsonFileOut.good() ) {
        l_jsonFileOut << std::setw( 4 ) << this->_json << std::endl;

    } else {
        try {
            l_jsonFileOut.exceptions( l_jsonFileOut.failbit );

        } catch ( const std::ios_base::failure& _jsonFileOutFailure ) {
            const std::error_code l_errorCode = _jsonFileOutFailure.code();
            const std::string l_what = _jsonFileOutFailure.what();

            // IC( l_errorCode.value(), l_what );
            // fmt::print( stderr, "{}: {}\n",
            //             l_errorCode.value(), l_what );
            printf( "%d: %s\n", l_errorCode.value(), l_what.c_str() );

            l_returnValue = l_errorCode.value();
        }
    }

    return ( l_returnValue );
}

uint16_t JSONFile::overwrite( json _json ) {
    uint16_t l_returnValue = 0;

    this->_json = _json;

    l_returnValue = this->overwrite();

    return ( l_returnValue );
}

json JSONFile::get( void ) {
    return ( this->_json );
}

json& JSONFile::operator[]( std::string _key ) {
    return ( this->_json[ _key ] );
}
