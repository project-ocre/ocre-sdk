#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#define THREAD_COUNT 4

void *thread_function(void *arg) {
    int id = *(int *)arg;
    for(int i = 0; i < 10; i++) {
        fprintf(stderr, "Hello from thread %d: %d\n", id, i);
        sleep(1);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t threads[THREAD_COUNT];
    int i;

    fprintf(stderr, "Starting pthread example\n");

    for (i = 0; i < THREAD_COUNT; i++) {
        int rc = pthread_create(&threads[i], NULL, thread_function, &i);
        fprintf(stderr, "%d:RC: %d\n", i, rc);
        if (rc) {
            perror("pthread_create");
            return 1;
        }
        usleep(100000);
    }

    fprintf(stderr, "Joining threads\n");

    for (i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            return 1;
        }
    }

    fprintf(stderr, "Finished pthread example\n");

    return 0;
}
