/*
 * @copyright Copyright © contributors to Project Ocre,
 * which has been established as Project Ocre a Series of LF Projects, LLC

 * SPDX-License-Identifier: Apache-2.0

 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DATA_SIZE 1000000  // 1MB of data
#define CHUNK_SIZE 1024    // Process in 1KB chunks
#define ITERATIONS 100     // Number of processing iterations

// Large static data arrays to increase binary size
static const char large_data_array1[200000] = {
    // Initialize with pattern data to prevent optimization
    [0] = 1, [1000] = 2, [2000] = 3, [3000] = 4, [4000] = 5,
    [5000] = 6, [6000] = 7, [7000] = 8, [8000] = 9, [9000] = 10,
    [10000] = 11, [15000] = 12, [20000] = 13, [25000] = 14, [30000] = 15,
    [40000] = 16, [50000] = 17, [60000] = 18, [70000] = 19, [80000] = 20,
    [90000] = 21, [100000] = 22, [110000] = 23, [120000] = 24, [130000] = 25,
    [140000] = 26, [150000] = 27, [160000] = 28, [170000] = 29, [180000] = 30,
    [190000] = 31, [199000] = 32, [199999] = 33
};

static const char large_data_array2[200000] = {
    [0] = 100, [1111] = 101, [2222] = 102, [3333] = 103, [4444] = 104,
    [5555] = 105, [6666] = 106, [7777] = 107, [8888] = 108, [9999] = 109,
    [11111] = 110, [22222] = 111, [33333] = 112, [44444] = 113, [55555] = 114,
    [66666] = 115, [77777] = 116, [88888] = 117, [99999] = 118, [111111] = 119,
    [122222] = 120, [133333] = 121, [144444] = 122, [155555] = 123, [166666] = 124,
    [177777] = 125, [188888] = 126, [199999] = 127
};

static const char large_data_array3[200000] = {
    [0] = 200, [777] = 201, [1555] = 202, [2333] = 203, [3111] = 204,
    [4999] = 205, [5777] = 206, [6555] = 207, [7333] = 208, [8111] = 209,
    [9999] = 210, [11777] = 211, [13555] = 212, [15333] = 213, [17111] = 214,
    [19999] = 215, [22777] = 216, [25555] = 217, [28333] = 218, [31111] = 219,
    [44444] = 220, [55555] = 221, [66666] = 222, [77777] = 223, [88888] = 224,
    [99999] = 225, [111111] = 226, [133333] = 227, [155555] = 228, [177777] = 229,
    [199999] = 230
};

static const char large_data_array4[200000] = {
    [123] = 42, [1234] = 43, [2345] = 44, [3456] = 45, [4567] = 46,
    [5678] = 47, [6789] = 48, [7890] = 49, [8901] = 50, [9012] = 51,
    [10123] = 52, [21234] = 53, [32345] = 54, [43456] = 55, [54567] = 56,
    [65678] = 57, [76789] = 58, [87890] = 59, [98901] = 60, [109012] = 61,
    [120123] = 62, [131234] = 63, [142345] = 64, [153456] = 65, [164567] = 66,
    [175678] = 67, [186789] = 68, [197890] = 69, [199012] = 70, [199999] = 71
};

// Large lookup table
static const int lookup_table[50000] = {
    [0] = 1000, [100] = 1001, [200] = 1002, [300] = 1003, [400] = 1004,
    [500] = 1005, [600] = 1006, [700] = 1007, [800] = 1008, [900] = 1009,
    [1000] = 1010, [2000] = 1020, [3000] = 1030, [4000] = 1040, [5000] = 1050,
    [6000] = 1060, [7000] = 1070, [8000] = 1080, [9000] = 1090, [10000] = 1100,
    [15000] = 1150, [20000] = 1200, [25000] = 1250, [30000] = 1300, [35000] = 1350,
    [40000] = 1400, [45000] = 1450, [49999] = 1499
};

// Generate test data and perform computations to create ~1MB output
int main()
{  
    setvbuf(stdout, NULL, _IONBF, 0); 
    printf("=== OCRE BIG SAMPLE - LARGE BINARY WITH DATA ARRAYS ===\n");
    printf("Binary contains %d bytes of static data arrays\n", 
           (int)(sizeof(large_data_array1) + sizeof(large_data_array2) + 
                sizeof(large_data_array3) + sizeof(large_data_array4) + 
                sizeof(lookup_table)));
    printf("Initializing large data processing...\n");

    // Use the static arrays to prevent compiler optimization
    printf("Static array checksums: %d, %d, %d, %d, lookup: %d\n",
           large_data_array1[0], large_data_array2[1111], large_data_array3[777],
           large_data_array4[123], lookup_table[100]);

    // Allocate memory for our big data processing
    char *buffer = malloc(DATA_SIZE);
    if (!buffer) {
        printf("ERROR: Failed to allocate %d bytes\n", DATA_SIZE);
        return 1;
    }

    // Fill buffer with pattern data
    printf("Filling buffer with test data...\n");
    for (int i = 0; i < DATA_SIZE; i++) {
        buffer[i] = (char)((i * 7 + 42) % 256);
    }

    printf("Starting data processing iterations...\n");
    
    // Perform multiple iterations of data processing
    for (int iter = 0; iter < ITERATIONS; iter++) {
        printf("\n--- ITERATION %d/%d ---\n", iter + 1, ITERATIONS);
        
        // Process data in chunks
        unsigned long checksum = 0;
        int zero_count = 0;
        int max_value = 0;
        int min_value = 255;
        
        for (int chunk = 0; chunk < DATA_SIZE / CHUNK_SIZE; chunk++) {
            int chunk_start = chunk * CHUNK_SIZE;
            unsigned char chunk_sum = 0;
            
            // Use static arrays in processing to prevent optimization
            int array_index = chunk % 199999;
            unsigned char static_byte = large_data_array1[array_index] ^ 
                                       large_data_array2[array_index] ^
                                       large_data_array3[array_index] ^
                                       large_data_array4[array_index];
            if (chunk < 50000) {
                static_byte ^= lookup_table[chunk];
            }
            
            // Analyze this chunk
            for (int i = 0; i < CHUNK_SIZE; i++) {
                unsigned char byte_val = (unsigned char)buffer[chunk_start + i];
                chunk_sum += byte_val;
                checksum += byte_val;
                
                if (byte_val == 0) zero_count++;
                if (byte_val > max_value) max_value = byte_val;
                if (byte_val < min_value) min_value = byte_val;
                
                // Perform some mathematical operations
                double sin_val = sin((double)byte_val / 255.0 * 3.14159);
                double cos_val = cos((double)byte_val / 255.0 * 3.14159);
                
                // Modify buffer based on calculations and static data
                buffer[chunk_start + i] = (char)((int)(sin_val * cos_val * 127 + 128 + static_byte) % 256);
            }
            
            // Print chunk statistics every 100 chunks
            if (chunk % 100 == 0) {
                printf("Chunk %d: sum=0x%02X, avg=%.2f, sin_transform_applied\n", 
                       chunk, chunk_sum, (double)chunk_sum / CHUNK_SIZE);
            }
        }
        
        printf("Iteration %d complete:\n", iter + 1);
        printf("  Total checksum: 0x%08lX\n", checksum);
        printf("  Zero bytes: %d\n", zero_count);
        printf("  Value range: %d - %d\n", min_value, max_value);
        printf("  Processing rate: %.2f MB/s (simulated)\n", 
               (double)DATA_SIZE / (1024 * 1024) / (0.1 * (iter + 1)));
        
        // Generate some hex dump output for verification
        printf("Sample data (first 256 bytes):\n");
        for (int row = 0; row < 16; row++) {
            printf("%04X: ", row * 16);
            for (int col = 0; col < 16; col++) {
                printf("%02X ", (unsigned char)buffer[row * 16 + col]);
            }
            printf(" |");
            for (int col = 0; col < 16; col++) {
                char c = buffer[row * 16 + col];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("|\n");
        }
        
        // Generate statistical analysis
        printf("\nStatistical Analysis:\n");
        int histogram[256] = {0};
        for (int i = 0; i < DATA_SIZE; i++) {
            histogram[(unsigned char)buffer[i]]++;
        }
        
        printf("Byte frequency distribution:\n");
        for (int i = 0; i < 256; i += 16) {
            printf("0x%02X-0x%02X: ", i, i + 15);
            for (int j = 0; j < 16; j++) {
                if (histogram[i + j] > 0) {
                    printf("%d ", histogram[i + j]);
                } else {
                    printf("0 ");
                }
            }
            printf("\n");
        }
        
        // Simulate some processing delay and output
        printf("Processing matrices and vectors...\n");
        for (int matrix = 0; matrix < 10; matrix++) {
            printf("Matrix %d transformation: ", matrix);
            for (int elem = 0; elem < 50; elem++) {
                double val = sin(matrix * 0.1 + elem * 0.05) * cos(elem * 0.1);
                printf("%.3f ", val);
                if ((elem + 1) % 10 == 0) printf("\n                              ");
            }
            printf("\n");
        }
    }
    
    // Final summary
    printf("\n=== PROCESSING COMPLETE ===\n");
    printf("Total data processed: %d bytes (%d KB)\n", 
           DATA_SIZE * ITERATIONS, (DATA_SIZE * ITERATIONS) / 1024);
    printf("Total output generated: ~1MB\n");
    printf("Buffer final checksum: ");
    unsigned long final_checksum = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        final_checksum += (unsigned char)buffer[i];
    }
    printf("0x%08lX\n", final_checksum);
    
    printf("\nMemory cleanup...\n");
    free(buffer);
    printf("Big sample execution completed successfully!\n");
    
    return 0;
}
