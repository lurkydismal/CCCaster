#pragma once

#include <stdint.h>
#include <stdio.h>

#define LOG_FILE_NAME "log"
#define LOG_FILE_EXTENSION "txt"
#define MAX_TRANSACITON_STRING_SIZE ( 1024 * 2 )

extern FILE* g_fileDescriptor;

uint16_t log_query( const char* _string );
uint16_t log_commit( void );
