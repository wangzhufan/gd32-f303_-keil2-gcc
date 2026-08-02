#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <stdint.h>
#include <stdbool.h>

#define RING_BUFF_SIZE 2056
typedef struct {
    uint8_t buffer[RING_BUFF_SIZE]; // Example size, adjust as needed
    volatile uint32_t head;
    volatile uint32_t tail;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *rb);
void ring_buffer_write(ring_buffer_t *rb, uint8_t data);
void ring_buffer_read(ring_buffer_t *rb, uint8_t *data);
void ring_buffer_clear(ring_buffer_t *rb);
void ring_buffer_get_count(ring_buffer_t *rb, uint32_t *count);
#endif