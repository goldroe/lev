#include <windows.h>
#include <shlwapi.h>

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

int concat_path(char *dest, size_t dest_size, char *src) {
    size_t d = 0;
    while (dest[d] != 0) {
        d++;
    }

    for (size_t s = 0; src[s] != 0; s++, d++) {
        if (d >= dest_size) return -1;

        dest[d] = src[s];
    }

    dest[d] = '\0';

    return 0;
}

void replace_backslash(char **path) {
    char *p = *path;
    size_t i = 0;
    while (p[i] != 0) {
        if (p[i] == '\\') p[i] = '/';
        i++;
    }
}

int main(int argc, char **argv) {
    DWORD len = GetCurrentDirectory(0, nullptr);
    char *current_path = (char *)malloc(len);
    DWORD nret = GetCurrentDirectory(len, current_path);
    assert(len == nret + 1);
    printf("Current Path: %s\n", current_path);

    char *find_path = nullptr;
    if (argc > 1) {
        char *arg_path = argv[1];
        char *path = (char *)calloc(strlen(arg_path) + 1, sizeof(char));
        strcpy(path, arg_path);

        if (PathIsRelativeA(path)) {
            size_t n = strlen(path) + strlen(current_path) + 1;
            path = (char *)realloc(path, n + 1);
            concat_path(path, n, "\\");
            concat_path(path, n, 
        } else {
            find_path = path;
        }
    } else {
        WIN32_FIND_DATAA find_data{};
        find_path = (char *)malloc(len + 2);
        strcpy(find_path, current_path);
        concat_path(find_path, len + 2, "\\*");
        printf("Find Path: %s\n", find_path);
        HANDLE find_handle = FindFirstFileA(find_path, &find_data);
    }

    do {
        char *file_name = find_data.cFileName;

        if (strcmp(file_name, ".") == 0 || strcmp(file_name, "..") == 0) continue;

        size_t path_len = strlen(current_path) + strlen(find_data.cFileName);
        char *file_path = (char *)malloc(path_len + 2);
        strcpy(file_path, current_path);
        concat_path(file_path, path_len + 1, "\\");
        concat_path(file_path, path_len + 1, find_data.cFileName);

        DWORD file_attributes = find_data.dwFileAttributes;
        if (file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
        }

        HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle == INVALID_HANDLE_VALUE) {
            // printf("error: %d\n", GetLastError());
            continue;
        }

        int64_t file_size = 0;
        GetFileSizeEx(file_handle, (LARGE_INTEGER *)&file_size);

        printf("%lld %s\n", file_size, file_name);
    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);


    return 0;
}
