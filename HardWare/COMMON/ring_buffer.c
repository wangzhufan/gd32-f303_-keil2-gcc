#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

void ring_buffer_write(ring_buffer_t *rb, uint8_t data) {
    uint32_t next_head = (rb->head + 1) % RING_BUFF_SIZE;
    if (next_head != rb->tail) { // Check for buffer full
        rb->buffer[rb->head] = data;
        rb->head = next_head;
    }
}

void ring_buffer_read(ring_buffer_t *rb, uint8_t *data) {
    if (rb->head != rb->tail) { // Check for buffer empty
        *data = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % RING_BUFF_SIZE;
    }
}

void ring_buffer_clear(ring_buffer_t *rb) {
    rb->head = 0;
    rb->tail = 0;
}

void ring_buffer_get_count(ring_buffer_t *rb, uint32_t *count) {
    if (rb->head >= rb->tail) {
        *count = rb->head - rb->tail;
    } else {
        *count = RING_BUFF_SIZE - rb->tail + rb->head;
    }
}