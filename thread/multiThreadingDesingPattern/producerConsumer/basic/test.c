#include "producer_consumer.h"
#include <pthread.h>
#include <stdlib.h>

int main() {
    shared_data_t shared;
    queue_init(&shared);

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    thread_arg_t args[NUM_PRODUCERS + NUM_CONSUMERS];

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].thread_id = i;
        args[i].shared = &shared;
        pthread_create(&producers[i], NULL, producer, &args[i]);
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        args[NUM_PRODUCERS + i].thread_id = i;
        args[NUM_PRODUCERS + i].shared = &shared;
        pthread_create(&consumers[i], NULL, consumer, &args[NUM_PRODUCERS + i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(producers[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(consumers[i], NULL);

    return 0;
}
