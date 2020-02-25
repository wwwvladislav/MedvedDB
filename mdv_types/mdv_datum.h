/**
 * @file mdv_datum.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Field data storage for given type.
 * @version 0.1
 * @date 2020-02-20
 *
 * @copyright Copyright (c) 2020, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_field.h"
#include "mdv_data.h"
#include <mdv_alloc.h>


/// Field data storage
typedef struct mdv_datum mdv_datum;


#define mdv_datum_constructor(T, name)  \
        mdv_datum * mdv_datum_##name##_create(T const *v, uint32_t count, mdv_allocator const *allocator);

mdv_datum_constructor(bool,         bool);
mdv_datum_constructor(char,         char);
mdv_datum_constructor(uint8_t,      byte);
mdv_datum_constructor(int8_t,       int8);
mdv_datum_constructor(uint8_t,      uint8);
mdv_datum_constructor(int16_t,      int16);
mdv_datum_constructor(uint16_t,     uint16);
mdv_datum_constructor(int32_t,      int32);
mdv_datum_constructor(uint32_t,     uint32);
mdv_datum_constructor(int64_t,      int64);
mdv_datum_constructor(uint64_t,     uint64);
mdv_datum_constructor(float,        float);
mdv_datum_constructor(double,       double);

#undef mdv_datum_constructor

mdv_datum * mdv_datum_create(mdv_data const *data, mdv_allocator const *allocator);
void        mdv_datum_free(mdv_datum *datum);
uint32_t    mdv_datum_size(mdv_datum *datum);
void *      mdv_datum_ptr(mdv_datum *datum);

