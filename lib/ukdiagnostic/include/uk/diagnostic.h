#include <uk/list.h>
#include <uk/config.h>
#include <uk/essentials.h>
#include <stdio.h>
#include <stdlib.h>
#include <uk/ctors.h>

struct diagnostic_entry {
    char *diag_name;
    int (*func)();
    struct uk_list_head next;
};

void add_entry(struct diagnostic_entry *entry);
int run_diag_function(char *name);

#define DIAGNOSTIC_FUNCTION(name,fn)  \
    static struct diagnostic_entry __STRINGCONCAT(DIAG_ENTRY__, fn) = { \
        .diag_name = name,\
        .func = &fn,   \
        .next = NULL    \
    }; \
    void __CONSTRUCTOR_NAME(fn)() { \
        printf("REGISTERED FUNCTION %s\n", name);   \
        add_entry(&__STRINGCONCAT(DIAG_ENTRY__, fn)); \
    } \
    UK_CTOR_FUNC(1, __CONSTRUCTOR_NAME(fn));

#define __STRINGCONCAT(x, y) x ## y

#define __CONSTRUCTOR_NAME(name) \
    __STRINGCONCAT(__CONSTRUCTOR__, name)


int headerfunc(){
    printf("yo!\n");
    return 1;
}

DIAGNOSTIC_FUNCTION("Diagnostic",headerfunc);
