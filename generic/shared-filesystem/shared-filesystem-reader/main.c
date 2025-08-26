#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define SHARED_FILE "shared/shared_data.txt"
#define BUF_SIZE 32

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0); 
    
    printf("Shared filesystem reader started\n");
    
    // Open file for reading
    FILE *file = fopen(SHARED_FILE, "r");
    if (!file) {
        printf("fopen failed for \"%s\": %s (%d)\n", SHARED_FILE, strerror(errno), errno);
        return -1;
    }
    printf("fopen success: %s\n", SHARED_FILE);
    
    // Read file content
    char buffer[BUF_SIZE];
    size_t nitems = fread(buffer, sizeof(char), sizeof(buffer) - 1, file);
    printf("fread: %zu bytes\n", nitems);
    
    // Null-terminate the buffer
    buffer[nitems] = '\0';
    printf("buffer read = %s\n", buffer);
    
    // Close the file
    int rc = fclose(file);
    printf("fclose returned %d\n", rc);
    
    printf("Reader completed successfully\n");
    fflush(stdout);
    
    return 0;
} 