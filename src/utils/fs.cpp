#include <string.h>
#include "utils/fs.h"
#include "assets_bundle.h"
#include "utils/log.h"

bool readFile(const char *filename, File *file)
{
    log_t("read file %s", filename);
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

bool readFile(const char *directory, const char *filename, File *file)
{
    size_t directoryLength = strlen(directory);
    size_t pathLength = directoryLength + strlen(filename);
    log_t("read file %s%s", directory, filename);
    for (uint32_t i = 0; i < fs_files_count; i++)
    {
        if (strlen(fs_files[i]) < pathLength)
            continue;
        if (!strncmp(fs_files[i], directory, directoryLength) && !strcmp(fs_files[i] + directoryLength, filename))
        {
            file->size = fs_files_content_lengths[i];
            file->content = fs_files_content[i];
            file->mime_type = fs_files_mime_types[i];
            return true;
        }
    }
    return false;
}