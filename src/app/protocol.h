#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H

#include <stddef.h>

#define INPUT_BLOCK_SIZE 4096

typedef enum {
    MESSAGE_DATA,
    MESSAGE_END,
    MESSAGE_RESULT,
    MESSAGE_OVERFLOW,
    MESSAGE_DONE
} message_kind_t;

typedef struct {
    message_kind_t kind;
    union {
        struct {
            size_t size;
            char bytes[INPUT_BLOCK_SIZE];
        } data;
        struct {
            int sum;
            int count;
            size_t line;
        } result;
    } payload;
} message_t;

#endif
