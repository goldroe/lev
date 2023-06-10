#include <windows.h>
#include <shlwapi.h>
#include <WinCon.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include <vector>

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

void path_grow(Path_Str *path, size_t num) {
    size_t old_cap = path->cap;
    size_t new_cap = 1;
    while (new_cap <= old_cap + num) {
        new_cap = 2 * new_cap + 1;
    }

    path->contents = (char *)realloc(path->contents, new_cap * sizeof(char));
    path->cap = new_cap;
}

void path_ensure_cap(Path_Str *path, size_t len) {
    if (path->cap < path->len + 1 + len) {
        path_grow(path, len);
    }
}

void path_char(Path_Str *path, char ch) {
    path_ensure_cap(path, 1);
    path->contents[path->len] = ch;
    path->contents[path->len+1] = '\0';
    path->len++;
}

void path_append(Path_Str *path, char *s) {
    size_t old_len = path->len;
    size_t str_len = strlen(s);
    path_ensure_cap(path, str_len);
    memcpy(path->contents + old_len, s, str_len);
    path->contents[old_len + str_len] = '\0';
    path->len = old_len + str_len;
}

void path_prepend(Path_Str *path, char *s) {
    size_t str_len = strlen(s);
    size_t old_len = path->len;
    path_ensure_cap(path, str_len);
    char *old_contents = (char *)malloc(old_len);
    memcpy(old_contents, path->contents, old_len);
    memcpy(path->contents, s, str_len);
    memcpy(path->contents + str_len, old_contents, old_len);
    path->contents[old_len + str_len] = '\0';
    path->len = old_len + str_len;
}

Path_Str path_copy(Path_Str path) {
    Path_Str result{};
    result.contents = (char *)malloc(path.cap);
    memcpy(result.contents, path.contents, path.cap);
    result.contents[path.len] = '\0';
    result.len = path.len;
    result.cap = path.cap;
    return result;
}

Path_Str path_init(char *path, size_t len) {
    Path_Str result{};
    result.contents = (char *)malloc(len + 1);
    memcpy(result.contents, path, len);
    result.contents[len] = '\0';
    result.len = len;
    result.cap = len + 1;
    return result;
}

HANDLE hc;
CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info;

void win32_print_directory_info(Path_Str directory_path) {
    Path_Str find_path = path_copy(directory_path);
    path_append(&find_path, "\\*");
    printf("%s find:%s\n", directory_path.contents, find_path.contents);
    WIN32_FIND_DATAA find_data{};
    HANDLE find_handle = FindFirstFileA(find_path.contents, &find_data);
    assert(find_handle != INVALID_HANDLE_VALUE);


    do {
        int64_t file_size = 0;
        char *file_name = find_data.cFileName;

        Path_Str file_path = path_copy(directory_path);
        path_append(&file_path, "\\");
        path_append(&file_path, file_name);
        
        DWORD file_attributes = find_data.dwFileAttributes;
        HANDLE file_handle = CreateFileA(file_path.contents, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file_handle != INVALID_HANDLE_VALUE) {
            GetFileSizeEx(file_handle, (LARGE_INTEGER *)&file_size);
        }

        printf("%lld ", file_size);

        if (file_attributes & FILE_ATTRIBUTE_DIRECTORY) {
            SetConsoleTextAttribute(hc, FOREGROUND_BLUE);
        }
        printf("%s\n", file_name);

        SetConsoleTextAttribute(hc, 7);

        CloseHandle(file_handle);
        free(file_path.contents);
    } while (FindNextFileA(find_handle, &find_data));

    free(find_path.contents);
}

int main(int argc, char **argv) {
    hc = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hc, &screen_buffer_info);

    Path_Str current_path{};
    {
        DWORD len = GetCurrentDirectory(0, nullptr);
        char *dir = (char *)malloc(len);
        DWORD nret = GetCurrentDirectory(len, dir);
        assert(len == nret + 1);
        current_path = path_init(dir, len - 1);
    }

    std::vector<Path_Str> arg_paths;
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (*arg == '-') continue;

        Path_Str path = path_init(arg, strlen(arg));
        arg_paths.push_back(path);
    }

    if (arg_paths.empty()) {
        win32_print_directory_info(current_path);
    }

    for (int i = 0; i < arg_paths.size(); i++) {
        Path_Str arg_path = arg_paths[i];
        if (PathIsRelativeA(arg_path.contents)) {
            Path_Str p = path_copy(current_path);
            path_append(&p, "\\");
            path_append(&p, arg_path.contents);
            free(arg_path.contents);
            arg_path = p;
        }

        win32_print_directory_info(arg_path);
    }

    SetConsoleTextAttribute(hc, screen_buffer_info.wAttributes);
    return 0;
}
