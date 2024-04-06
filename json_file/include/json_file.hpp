#pragma once

#include <string>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

class JSONFile {
public:
    JSONFile( void );
    JSONFile( std::string const& _fileName, std::string const& _fileExtension );

    uint16_t overwrite( void );
    uint16_t overwrite( json _json );

    json get( void );
    json& operator[]( std::string _key );

private:
    std::string _fileName;
    std::string _fileExtension;
    std::string _filePath;

    json _json;
};
