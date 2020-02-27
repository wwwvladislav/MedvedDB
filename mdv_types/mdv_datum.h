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


/// Field data storage
typedef struct mdv_datum mdv_datum;


mdv_datum * mdv_datum_bool_create(bool const *v, uint32_t count);
mdv_datum * mdv_datum_char_create(char const *v, uint32_t count);
mdv_datum * mdv_datum_byte_create(uint8_t const *v, uint32_t count);

mdv_datum * mdv_datum_int8_create(int8_t const *v, uint32_t count);
mdv_datum * mdv_datum_uint8_create(uint8_t const *v, uint32_t count);

mdv_datum * mdv_datum_int16_create(int16_t const *v, uint32_t count);
mdv_datum * mdv_datum_uint16_create(uint16_t const *v, uint32_t count);

mdv_datum * mdv_datum_int32_create(int32_t const *v, uint32_t count);
mdv_datum * mdv_datum_uint32_create(uint32_t const *v, uint32_t count);

mdv_datum * mdv_datum_int64_create(int64_t const *v, uint32_t count);
mdv_datum * mdv_datum_uint64_create(uint64_t const *v, uint32_t count);

mdv_datum * mdv_datum_float_create(float const *v, uint32_t count);
mdv_datum * mdv_datum_double_create(double const *v, uint32_t count);


mdv_datum *     mdv_datum_create(mdv_field_type type, void const *v, uint32_t size);
void            mdv_datum_free(mdv_datum *datum);
uint32_t        mdv_datum_size(mdv_datum const *datum);
void *          mdv_datum_ptr(mdv_datum const *datum);
mdv_field_type  mdv_datum_type(mdv_datum const *datum);
