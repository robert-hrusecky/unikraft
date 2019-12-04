#include <uk/list.h>
#include <uk/config.h>
#include <uk/essentials.h>

struct diagnostic_entry {
    char *diag_name;
    int (*func)();
    struct uk_list_head next;
};

void add_entry(struct diagnostic_entry *entry);
int run_diag_function(char *name);
