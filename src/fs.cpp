#include <string.h>
#include "fs.h"
#include "assets_bundle.h"
#include "log.h"

bool readFile(const char *filename, File *file)
{
    for (uint32_t i = 0; i < fs_files_count; i++)
    {
        if (!strcmp(fs_files[i], filename))
        {
            file->size = fs_files_content_lengths[i];
            file->content = fs_files_content[i];
            file->mime_type = fs_files_mime_types[i];
            return true;
        }
    }
    return false;
}