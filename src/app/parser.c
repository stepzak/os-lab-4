#include "app/parser.h"

#include <ctype.h>
#include <limits.h>

static void reset_number(parser_t *parser) {
    parser->value = 0;
    parser->sign = 1;
    parser->has_digits = false;
    parser->has_sign = false;
}

void parser_init(parser_t *parser) {
    *parser = (parser_t){.sign = 1, .line = 1};
}

bool parser_flush(parser_t *parser) {
    if (!parser->has_digits) {
        reset_number(parser);
        return true;
    }
    int64_t number = parser->value * parser->sign;
    if ((number > 0 && parser->sum > INT_MAX - number) ||
        (number < 0 && parser->sum < INT_MIN - number) ||
        parser->count == INT_MAX)
        return false;
    parser->sum += number;
    ++parser->count;
    reset_number(parser);
    return true;
}

bool parser_feed(parser_t *parser, char ch) {
    if (isspace((unsigned char)ch)) return parser_flush(parser);
    if ((ch == '-' || ch == '+') && !parser->has_digits && !parser->has_sign) {
        parser->sign = ch == '-' ? -1 : 1;
        parser->has_sign = true;
        return true;
    }
    if (isdigit((unsigned char)ch)) {
        int digit = ch - '0';
        int64_t limit = parser->sign < 0 ? -(int64_t)INT_MIN : INT_MAX;
        if (parser->value > (limit - digit) / 10) return false;
        parser->value = parser->value * 10 + digit;
        parser->has_digits = true;
        return true;
    }
    return parser_flush(parser);
}

bool parser_take_line(parser_t *parser, message_t *result) {
    bool present = parser->count != 0;
    *result = (message_t){
        .kind = MESSAGE_RESULT,
        .payload.result = {.sum = (int)parser->sum, .count = parser->count,
                           .line = parser->line},
    };
    reset_number(parser);
    parser->sum = 0;
    parser->count = 0;
    ++parser->line;
    return present;
}
