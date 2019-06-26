#pragma once
#include <stddef.h>


typedef struct mdv_deque mdv_deque;


mdv_deque * mdv_deque_create(size_t entry_size, size_t size);
void        mdv_deque_free(mdv_deque *deque);
