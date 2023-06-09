#include <windows.h>
#include <shlwapi.h>
#include <WinCon.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

// NOTE: null terminated string with length
// len does not include the null terminator
// capacity does include the null terminator
typedef struct {
    char *contents;
    size_t len;
    size_t cap;
} Path_Str;

typedef struct {
    char *file_name;
    char *path;
    int64_t file_size;
    // int attributes?
} File_Info;

typedef struct Directory_Node Directory_Node;
struct Directory_Node {
    char *name;
    char *path;

    File_Info **files;
    size_t file_num;
    struct Directory_Node **children;
    size_t child_num;
};

void path_str_grow(Path_Str *path, size_t num) {
    size_t old_cap = path->cap;
    size_t new_cap = 1;
    while (new_cap <= old_cap + num) {
        new_cap = 2 * new_cap + 1;
    }

    path->contents = (char *)realloc(path->contents, new_cap * sizeof(char));
    path->cap = new_cap;
}

void path_str_ensure_cap(Path_Str *path, size_t len) {
    if (path->cap < path->len + 1 + len) {
        path_str_grow(path, len);
    }
}

void path_str_append_char(Path_Str *path, char ch) {
    path_str_ensure_cap(path, 1);
    path->contents[path->len] = ch;
    path->contents[path->len+1] = '\0';
    path->len++;
}

void path_str_append(Path_Str *path, char *s) {
    size_t old_len = path->len;
    size_t str_len = strlen(s);
    path_str_ensure_cap(path, str_len);
    memcpy(path->contents + old_len, s, str_len);
    path->contents[old_len + str_len] = '\0';
    path->len = old_len + str_len;
}

Path_Str path_str_copy(Path_Str path) {
    Path_Str result{};
    result.contents = (char *)malloc(path.cap);
    memcpy(result.contents, path.contents, path.cap);
    result.len = path.len;
    result.cap = path.cap;
    return result;
}

Path_Str path_str_init(char *path, size_t len) {
    Path_Str result{};
    result.contents = (char *)malloc(len + 1);
    memcpy(result.contents, path, len);
    result.contents[len] = '\0';
    result.len = len;
    result.cap = len;
    return result;
}

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
    HANDLE hc = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;
    GetConsoleScreenBufferInfo(hc, &screen_buffer_info);

    DWORD len = GetCurrentDirectory(0, nullptr);
    char *current_path = (char *)malloc(len);
    DWORD nret = GetCurrentDirectory(len, current_path);
    assert(len == nret + 1);
   
    char *path = nullptr;
    if (argc > 1) {
        char *arg_path = argv[1];
        path = (char *)calloc(strlen(arg_path) + 1, sizeof(char));

        if (PathIsRelativeA(arg_path)) {
            printf("Is reltiave\n");
            size_t n = strlen(arg_path) + strlen(current_path) + 1;
            path = (char *)realloc(path, n + 1);
            strcpy(path, current_path);
            concat_path(path, n, "\\");
            concat_path(path, n, arg_path);
        } else {
            printf("Is absolute\n");
            strcpy(path, arg_path);
        }
    } else {
        path = (char *)calloc(strlen(current_path) + 1, sizeof(char));
        strcpy(path, current_path);
    }

    char *find_path = (char *)calloc(strlen(path) + 2, sizeof(char));
    strcpy(find_path, path);
    concat_path(find_path, strlen(path) + 2, "\\*");

    printf("Find Path: %s\n", find_path);

    WIN32_FIND_DATAA find_data{};
    HANDLE find_handle = FindFirstFileA(find_path, &find_data);
    assert(find_handle != INVALID_HANDLE_VALUE);

    printf("%s:\n", path);
    do {
        int64_t file_size = 0;
        char *file_name = find_data.cFileName;

        size_t path_len = strlen(path) + strlen(find_data.cFileName);
        char *file_path = (char *)malloc(path_len + 2);
        strcpy(file_path, path);
        concat_path(file_path, path_len + 1, "\\");
        concat_path(file_path, path_len + 1, find_data.cFileName);

        DWORD file_attributes = find_data.dwFileAttributes;
        HANDLE file_handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle != INVALID_HANDLE_VALUE) {
            GetFileSizeEx(file_handle, (LARGE_INTEGER *)&file_size);
        }

        printf("%lld ", file_size);

        if (file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
            SetConsoleTextAttribute(hc, FOREGROUND_BLUE);
        }
        printf("%s path:%s\n", file_name, file_path);

        SetConsoleTextAttribute(hc, 7);

        CloseHandle(file_handle);
        free(file_path);
    } while (FindNextFileA(find_handle, &find_data));

    FindClose(find_handle);

    SetConsoleTextAttribute(hc, screen_buffer_info.wAttributes);
    return 0;
}
