/*
    fileIO.c, stdio helper for host

    provide orbisFileGetFileContent (old liborbisFile) to
    read file content in a buffer and set last filesize
*/

#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include <user_mem.h> 

size_t _orbisFile_lastopenFile_size;
// --------------------------------------------------------- buf_from_file ---
unsigned char *orbisFileGetFileContent( const char *filename )
{
    _orbisFile_lastopenFile_size = -1;

    FILE *file = fopen( filename, "rb" );
    if( !file )
        {log_error( "Unable to open file \"%s\".", filename ); return NULL; }

    fseek( file, 0, SEEK_END );
    size_t size = ftell( file );
    fseek(file, 0, SEEK_SET );

    unsigned char *buffer = (unsigned char *) malloc( (size +1) * sizeof(char) );
    fread( buffer, sizeof(char), size, file );
    buffer[size] = 0;
    _orbisFile_lastopenFile_size = size;
    fclose( file );

    return buffer;
}
