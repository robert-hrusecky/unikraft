#ifndef JSON_FRONTEND_H_
#define JSON_FRONTEND_H_

#include <stddef.h>
#include <stdint.h>

typedef enum {
    VALUE_OBJECT,
    VALUE_ARRAY,
    VALUE_STRING,
    VALUE_INT,
    VALUE_FLOAT,
    VALUE_TRUE,
    VALUE_FALSE,
    VALUE_NULL,
} VALUE_TYPE;

struct object {
    char* key;
    struct value* value;
    struct object* next;
};

struct array {
    struct value* value;
    struct array* next;
};

struct value {
    VALUE_TYPE type;
    union {
        struct object* object;
        struct array* array;
        char* string;
        int64_t integer;
        double floating;
    };
};

struct json {
    struct value* element;
};


struct json* parse_json(const char* data, size_t len);


#endif
