#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#define SHARED_DIR "/shared"
#define SHARED_FILE "/shared/shared_data.txt"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); 
    
    printf("Shared filesystem writer started\n");
    
    int rc;
    struct stat st;
    
    // Check if shared directory exists
    rc = stat(SHARED_DIR, &st);
    if (rc == 0 && S_ISDIR(st.st_mode)) {
        printf("Directory exists: %s\n", SHARED_DIR);
    }
    else if (rc != 0 && errno != ENOENT) {
        // stat() failed for unexpected reason
        printf("stat failed for path \"%s\": %s (%d)\n", SHARED_DIR, strerror(errno), errno);
        return -1;
    }
    else {
        // Directory doesn't exist, create it
        rc = mkdir(SHARED_DIR, 0777);
        if (rc != 0) {
            printf("mkdir failed for \"%s\": %s (%d)\n", SHARED_DIR, strerror(errno), errno);
            return -1;
        }
        printf("mkdir success: %s\n", SHARED_DIR);
    }
    
    // Open file for writing
    FILE *file = fopen(SHARED_FILE, "w");
    if (!file) {
        printf("fopen failed for \"%s\": %s (%d)\n", SHARED_FILE, strerror(errno), errno);
        return -1;
    }
    printf("fopen success: %s\n", SHARED_FILE);
    
    // Write "Hello World" to file
    const char *data = "Hello World";
    size_t nitems = fwrite(data, sizeof(char), strlen(data), file);
    printf("fwrite returned %zu bytes\n", nitems);
    
    // Close the fileg
    rc = fclose(file);
    printf("fclose returned %d\n", rc);
    
    printf("Writer completed successfully\n");
    fflush(stdout);
    
    return 0;
} 