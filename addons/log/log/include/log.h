#pragma once

#include <stdint.h>
#include <stdio.h>

#define LOG_FILE_NAME "log"
#define LOG_FILE_EXTENSION "txt"

extern FILE* g_fileDescriptor;

uint16_t log_query( const char* _string, const size_t _stringLength );
uint16_t log_commit( void );
