#include "board_field_type.h"
#include <stdlib.h>
#include <errno.h>

void initialize_field(field_t *field, uint32_t x, uint32_t y, uint32_t player) {
    field->x = x;
    field->y = y;
    field->owner = player;
    field->rep = field;
    field->rank = 0;
}

void* allocate_memory(size_t size, bool *error) {
    void *ptr = NULL;
    ptr = malloc(size);
    if (ptr == NULL) {
        errno = ENOMEM;
        *error = true;
    }
    else {
        *error = false;
    }
    return ptr;
}

field_t* find_root(field_t *field) {
    if (field->rep == field)
        return field;
    return find_root(field->rep);
}

void unite(field_t first, field_t second) {
    field_t *first_root = find_root(&first);
    field_t *second_root = find_root(&second);

    if (first_root == second_root)
        return;
    if (first_root->rank > second_root->rank)
        second_root->rep = first_root;
    else if (first_root->rank < second_root->rank)
        first_root->rep = second_root;
    else {
        second_root->rep = first_root;
        ++(first_root->rank);
    }
}

void set_field(field_t *field, uint32_t x, uint32_t y, uint32_t player) {
    field->x = x;
    field->y = y;
    field->owner = player;
}
