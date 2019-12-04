#include <uk/json_frontend.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static struct json_value* parse_value(const char* data, const size_t len, size_t* pos);
static char* parse_string(const char* data, const size_t len, size_t* pos);

static void parse_ws(const char* data, const size_t len, size_t* pos) {
    while (*pos < len && isspace(data[*pos])) {
        *pos++;
    }
}

static struct json_value* parse_element(const char* data, const size_t len, size_t* pos) {
    parse_ws(data, len, pos);
    struct json_value* value = parse_value(data, len, pos);
    parse_ws(data, len, pos);
    return value;
}

static struct json_object* parse_member(const char* data, const size_t len, size_t* pos) {
    parse_ws(data, len, pos);
    char* key = parse_string(data, len, pos);
    parse_ws(data, len, pos);
    *pos++; // :
    struct json_value* value = parse_element(data, len, pos);
    struct json_object* object = malloc(sizeof(struct json_object));
    object->key = key;
    object->value = value;
    object->next = NULL;
    return object;
}

static struct json_object* parse_object(const char* data, const size_t len, size_t* pos) {
    *pos++; // {
    parse_ws(data, len, pos);
    if (*pos >= len || data[*pos] == '}') {
        // This is an empty object
        *pos++; // }
        return NULL;
    }
    // Otherwise, we have a head member
    struct json_object* head = parse_member(data, len, pos);
    struct json_object* curr = head;
    while(data[*pos] != '}') {
        // get the next member, and append to the list
        curr->next = parse_member(data, len, pos);
        curr = curr->next;
    }
    *pos++; // }

}

static struct json_array* parse_array(const char* data, const size_t len, size_t* pos) {
    return NULL;
}

static char* parse_string(const char* data, const size_t len, size_t* pos) {
    return NULL;
}

static int64_t parse_int(const char* data, const size_t len, size_t* pos) {
    return 0;
}

static struct json_value* create_json_value() {
    struct json_value* value = malloc(sizeof(struct json_value));
    value->type = JSON_ERROR;
    return value;
}

static struct json_value* parse_value(const char* data, const size_t len, size_t* pos) {
    if (*pos >= len) return NULL;
    struct json_value* value = create_json_value(JSON_NULL);
    char next_char = data[*pos];
    // object: '{'
    // array: '['
    // string: '"'
    // true
    // false
    // null
    // number: else
    switch (next_char) {
        case '{':
            value->type = JSON_OBJECT;
            value->object = parse_object(data, len, pos);
            break;
        case '[':
            value->type = JSON_ARRAY;
            value->array = parse_array(data, len, pos);
            break;
        case '"':
            value->type = JSON_STRING;
            value->string = parse_string(data, len, pos);
            break;
        case 't':
            value->type = JSON_TRUE;
            *pos += 4;
            break;
        case 'f':
            value->type = JSON_FALSE;
            *pos += 5;
            break;
        case 'n':
            value->type = JSON_NULL;
            *pos += 4;
            break;
        default:
            value->type = JSON_INT;
            value->integer = parse_int(data, len, pos);
    }
    return value;
}


struct json_value* parse_json(const char* data, const size_t len) {
    printf("Testing\n");
    size_t pos = 0;
    return parse_value(data, len, &pos);
}
