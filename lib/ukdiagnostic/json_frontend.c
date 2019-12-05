#include <uk/json_frontend.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

static void free_json_object(struct json_object* object);
static void free_json_array(struct json_array* array);
static struct json_value* parse_value(const char* data, const size_t len, size_t* pos);
static char* parse_string(const char* data, const size_t len, size_t* pos);

static bool check_end(size_t len, size_t pos) {
    bool good = pos < len;
    if (!good) {
        printf("Unexpectedly reached end of json data\n");
    }
    return good;
}

static bool check_char(const char* data, size_t len, size_t pos, char check) {
    if (!check_end(len, pos))
        return false;
    bool good = data[pos] == check;
    if (!good) {
        printf("Unexpected character '%c' in json; expected '%c'\n", data[pos], check);
    }
    return good;
}

static void parse_ws(const char* data, const size_t len, size_t* pos) {
    while (*pos < len && isspace(data[*pos])) {
        (*pos)++;
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
    if (!check_char(data, len, *pos, ':')) {
        free(key);
        return NULL;
    }
    (*pos)++; // :
    struct json_value* value = parse_element(data, len, pos);
    struct json_object* object = malloc(sizeof(struct json_object));
    object->key = key;
    object->value = value;
    object->next = NULL;
    return object;
}

static struct json_object* parse_object(const char* data, const size_t len, size_t* pos) {
    if (!check_char(data, len, *pos, '{')) {
        return NULL;
    }
    (*pos)++; // {
    parse_ws(data, len, pos);
    if (*pos >= len || data[*pos] == '}') {
        // This is an empty object (unless EOF)
        check_end(len, *pos);
        (*pos)++; // }
        return NULL;
    }
    // Otherwise, we have a head member
    struct json_object* head = parse_member(data, len, pos);
    struct json_object* curr = head;
    while(data[*pos] != '}') {
        if (!check_char(data, len, *pos, ',')) {
            free_json_object(head);
            return NULL;
        }
        (*pos)++; // ,
        // get the next member, and append to the list
        curr->next = parse_member(data, len, pos);
        curr = curr->next;
    }
    if (!check_char(data, len, *pos, '}')) {
        free_json_object(head);
        return NULL;
    }
    (*pos)++; // }
    return head;
}

static struct json_array* parse_array(const char* data, const size_t len, size_t* pos) {
    return NULL;
}

static char* parse_string(const char* data, const size_t len, size_t* pos) {
    if (!check_char(data, len, *pos, '"'))
        return NULL;
    (*pos)++; // "

    // first count the chars
    size_t count;
    size_t i;
    for (i = *pos, count = 0; i < len && data[i] != '"'; i++, count++) {
        if (data[i] == '\\') {
            i++;
        }
    }

    if (!check_end(len, i)) {
        return NULL;        
    }

    char* string = malloc(sizeof(char) * (count + 1));
    for (size_t str_pos = 0; data[*pos] != '"'; (*pos)++, str_pos++) {
        if (data[*pos] == '\\') {
            (*pos)++;
        }
        string[str_pos] = data[*pos];
    }
    string[count] = '\0';
    if (!check_char(data, len, *pos, '"'))
        return NULL;
    (*pos)++; // "
    return string;
}

static int64_t parse_int(const char* data, const size_t len, size_t* pos) {
    if (!check_end(len, *pos))
        return 0;
    int negative = data[*pos] == '-';
    if (negative)
        (*pos)++; // -

    int64_t num = 0;
    for (; *pos < len && isdigit(data[*pos]); (*pos)++) {
        num *= 10;
        num += data[*pos] - '0';
    }
    if (negative)
        num *= -1;
    return num;
}

static struct json_value* create_json_value() {
    struct json_value* value = malloc(sizeof(struct json_value));
    value->type = JSON_ERROR;
    return value;
}

static struct json_value* parse_value(const char* data, const size_t len, size_t* pos) {
    if (!check_end(len, *pos))
        return NULL;
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
    size_t pos = 0;
    return parse_value(data, len, &pos);
}

static void free_json_object(struct json_object* object) {
    if (object == NULL) return;
    free(object->key);
    free_json_value(object->value);
    free_json_object(object->next);
    free(object);
}

static void free_json_array(struct json_array* array) {
    if (array == NULL) return;
    free_json_value(array->value);
    free_json_array(array->next);
    free(array);
}

void free_json_value(struct json_value* value) {
    switch (value->type) {
        case JSON_OBJECT:
            free_json_object(value->object);
            break;
        case JSON_ARRAY:
            free_json_array(value->array);
            break;
        case JSON_STRING:
            free(value->string);
            break;
        default:
            break;
    }
    free(value);
}

struct json_value* json_object_lookup(struct json_value* object_value, const char* key) {
    if (object_value->type != JSON_OBJECT) {
        return NULL;
    }
    for (struct json_object* curr = object_value->object; curr != NULL; curr = curr->next) {
        if (strcmp(curr->key, key) == 0)
            return curr->value;
    }
    return NULL;
}
