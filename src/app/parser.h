#ifndef APP_PARSER_H
#define APP_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/protocol.h"

typedef struct {
    int64_t value;
    int64_t sum;
    int sign;
    int count;
    bool has_digits;
    bool has_sign;
    size_t line;
} parser_t;

void parser_init(parser_t *parser);
bool parser_feed(parser_t *parser, char ch);
bool parser_flush(parser_t *parser);
bool parser_take_line(parser_t *parser, message_t *result);

#endif
