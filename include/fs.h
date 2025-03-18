

#include <stdint.h>

#ifndef FS_H
#define FS_H

struct File
{
    uint32_t size;
    const uint8_t *content;
    const char *mime_type;
};

bool readFile(const char *filename, File *file);

#endif // FS_H