#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

// #include <wasi/api.h>

// We are expecting to work at the root dir
#define CWD "/"
#define FOLDER_PATH CWD "folder"
#define FILE_PATH   CWD "folder/test.txt"
#define BUF_SIZE 512

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);             // Logs don't show up reliably so disable stdout buffering

    int rc;
    struct stat st;

    printf("Wasm filesystem demo\n");

    // Check if directory exists
    rc = stat(FOLDER_PATH, &st);
    if (rc == 0 && S_ISDIR(st.st_mode)) {
        printf("Directory exists: %s\n", FOLDER_PATH);

        // Attempt to delete test file
        rc = unlink(FILE_PATH);
        if (rc != 0 && errno != ENOENT) {
            printf("Failed to remove file \"%s\": %s (%d)\n", FILE_PATH, strerror(errno), errno);
            return -1;
        }

        // Attempt to remove directory
        rc = rmdir(FOLDER_PATH);
        if (rc != 0) {
            printf("Failed to remove directory \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
            return -1;
        }

        printf("Existing directory removed.\n");
    }
    else if (rc != 0 && errno != ENOENT) {
        // stat() failed for unexpected reason
        printf("stat failed for path \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
        return -1;
    }

    // Now create the directory
    rc = mkdir(FOLDER_PATH, 0777);
    if (rc != 0) {
        printf("mkdir failed for \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
        return -1;
    }
    printf("mkdir success: %s\n", FOLDER_PATH);

    // Open test file
    FILE *file = fopen(FILE_PATH, "w+");
    if (!file) {
        printf("fopen failed for \"%s\": %s (%d)\n", FILE_PATH, strerror(errno), errno);
        return -1;
    }
    printf("fopen success: %s\n", FILE_PATH);

    // Write to test file
    const char *data = "Hello, World!";
    size_t nitems = fwrite(data, sizeof(char), strlen(data) + 1, file);
    printf("fwrite returned %zu bytes\n", nitems);

    // Read file back
    rc = fseek(file, 0, SEEK_SET);
    printf("fseek returned %d\n", rc);

    char buffer[32];
    nitems = fread(buffer, sizeof(char), sizeof(buffer), file);
    printf("fread: %zu bytes\n", nitems);
    printf("buffer read = %s\n", buffer);

    // Close the file
    rc = fclose(file);
    printf("fclose returned %d\n", rc);

    // List contents of the directory
    printf("opendir: %s\n", FOLDER_PATH);
    DIR *dir = opendir(FOLDER_PATH);
    if (!dir) {
        printf("opendir failed for \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
        return -1;
    }
    printf("opendir OK! dir = %p\n", dir);

    printf("Directory listing for: %s\n", FOLDER_PATH);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("  %s (type: %d)\n", entry->d_name, entry->d_type);
    }

    printf("closedir\n");
    fflush(stdout);
    int closed = closedir(dir);
    if (closed != 0) {
        printf("closedir failed: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}
