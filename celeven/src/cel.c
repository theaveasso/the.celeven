#include "cel.h"

#include <stdio.h>
#include <string.h>

const char *cel_filename_from_path(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

void celfs_join_path(char *out, size_t out_size, const char *base, const char *relative_path) {
    size_t base_len = strlen(base);
    while (base_len > 0 && (base[base_len - 1] == '/' || base[base_len - 1] == '\\'))
    {
        base_len--;
    }

    snprintf(out, out_size, "%.*s%c%s", (int) base_len, base, FS_PATH_SEP, relative_path);
}

int celfs_set_current_dir(const char *path) {
    if (fs_chdir(path) == 0) { return 1; }
    perror("celfs_set_current_dir failed");
    return 0;
}

int celfs_get_current_dir(char *buffer, size_t size) {
    if (fs_getcwd(buffer, (int) size)) { return 1; }
    perror("celfs_get_current_dir failed");
    return 0;
}

CELAPI int celfs_resolve_full_path(char *out, size_t out_size, const char *relative_path, const char *path) {
    snprintf(out, out_size, "%s/%s", path, relative_path);
    FILE *f;
    fopen_s(&f, out, "rb");
    if (f)
    {
        fclose(f);
        return 1;
    }
    return 0;
}

void celfs_get_exec_dir(char *out, size_t out_size) {
}
