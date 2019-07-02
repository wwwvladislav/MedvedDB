#pragma once
#include <stddef.h>
#include <stdbool.h>


typedef struct mdv_deque mdv_deque;


mdv_deque * mdv_deque_create(size_t entry_size);
void        mdv_deque_free(mdv_deque *deque);
bool        mdv_deque_push_back(mdv_deque *deque, void const *e, size_t size);
bool        mdv_deque_pop_back(mdv_deque *deque, void *e);

