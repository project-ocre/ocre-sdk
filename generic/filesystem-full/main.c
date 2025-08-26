#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

#define CWD "/"
#define FOLDER_PATH CWD "folder"
#define FILE_PATH   CWD "folder/test.txt"
#define BUF_SIZE 512

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);  // Disable stdout buffering

    int rc;
    struct stat st;

    printf("LibC / POSIX File API Demo\n");

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
        printf("stat failed for path \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
        return -1;
    }

    // Create the directory
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

    // Write using various methods
    fwrite("Hello, World!\n", sizeof(char), 14, file);
    fputc('A', file);
    fputs("Line of text\n", file);
    fprintf(file, "Formatted number: %d\n", 42);
    fflush(file);

    // Rewind and read back
    rewind(file);

    char buffer[BUF_SIZE];
    fread(buffer, sizeof(char), 14, file);
    buffer[14] = '\0';
    printf("fread: %s\n", buffer);

    rewind(file);
    int ch = fgetc(file);
    printf("fgetc: %c\n", ch);

    rewind(file);
    fgets(buffer, sizeof(buffer), file);
    printf("fgets: %s\n", buffer);

    rewind(file);
    int num = 0;
    fscanf(file, "%*s %*s %d", &num);
    printf("fscanf: %d\n", num);

    // File positioning
    fseek(file, 0, SEEK_END);
    long pos = ftell(file);
    printf("ftell: %ld\n", pos);

    rewind(file);
    fpos_t fpos;
    if (fgetpos(file, &fpos) == 0) {
        printf("fgetpos succeeded\n");
    }
    if (fsetpos(file, &fpos) == 0) {
        printf("fsetpos succeeded\n");
    }

    // File status and control
    printf("feof: %d\n", feof(file));
    printf("ferror: %d\n", ferror(file));
    clearerr(file);
    fflush(file);

    // Try fileno (may not be supported)
    int fd = fileno(file);
    printf("fileno: %d\n", fd);

    // Try freopen (may not be supported)
    file = freopen(FILE_PATH, "r", file);
    if (!file) {
        printf("freopen failed: %s (%d)\n", strerror(errno), errno);
    } else {
        printf("freopen succeeded\n");
    }

    rc = fclose(file);
    printf("fclose returned %d\n", rc);

    // List contents of the directory
    printf("opendir: %s\n", FOLDER_PATH);
    DIR *dir = opendir(FOLDER_PATH);
    if (!dir) {
        printf("opendir failed for \"%s\": %s (%d)\n", FOLDER_PATH, strerror(errno), errno);
        return -1;
    }

    printf("Directory listing for: %s\n", FOLDER_PATH);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("  %s (type: %d)\n", entry->d_name, entry->d_type);
    }

    rc = closedir(dir);
    if (rc != 0) {
        printf("closedir failed: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}
