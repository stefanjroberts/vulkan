#include "core.h"

void *open_file(const char *file_name, i32 *file_size)
{
    FILE *fptr = fopen(file_name, "rb");
    fseek(fptr, 0L, SEEK_END);
    *file_size = ftell(fptr);
    fseek(fptr, 0L, SEEK_SET);
    void *file_contents = malloc(*file_size);
    fread(file_contents, 1, *file_size, fptr);
    fclose(fptr);
    return file_contents;
}

void close_file(void *file)
{
    free(file);
    return;
}
