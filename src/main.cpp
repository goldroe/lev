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

    char *path = nullptr;
    if (argc > 1) {
        char *arg_path = argv[1];
        path = (char *)calloc(strlen(arg_path) + 1, sizeof(char));

        if (PathIsRelativeA(path)) {
            size_t n = strlen(arg_path) + strlen(current_path) + 1;
            path = (char *)realloc(path, n + 1);
            strcpy(path, current_path);
            concat_path(path, n, "\\");
            concat_path(path, n, arg_path);
        } else {
            strcpy(path, arg_path);
        }
    } else {
        path = (char *)calloc(strlen(current_path) + 1, sizeof(char));
        strcpy(path, current_path);
    }

    char *find_path = (char *)calloc(strlen(path) + 2, sizeof(char));
    strcpy(find_path, path);
    concat_path(find_path, strlen(path) + 2, "\\*");

    WIN32_FIND_DATAA find_data{};
    HANDLE find_handle = FindFirstFileA(find_path, &find_data);
    assert(find_handle != INVALID_HANDLE_VALUE);

    printf("%s:\n", path);
    do {
        int64_t file_size = 0;
        char *file_name = find_data.cFileName;

        size_t path_len = strlen(current_path) + strlen(find_data.cFileName);
        char *file_path = (char *)malloc(path_len + 2);
        strcpy(file_path, current_path);
        concat_path(file_path, path_len + 1, "\\");
        concat_path(file_path, path_len + 1, find_data.cFileName);

        DWORD file_attributes = find_data.dwFileAttributes;
        if (file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
            
        }

        HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle != INVALID_HANDLE_VALUE) {
            GetFileSizeEx(file_handle, (LARGE_INTEGER *)&file_size);
        }

        printf("%lld  %-s\n", file_size, file_name);

        CloseHandle(file_handle);
        free(file_path);
    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);

    return 0;
}
