%module mdv

%inline %{
#include <mdv_datum.h>
%}

%include "stdint.i"
%include "mdv_array.i"

%mdv_array(bool,        ArrayOfBool);
%mdv_array(char,        ArrayOfChar);
%mdv_array(uint8_t,     ArrayOfByte);
%mdv_array(uint8_t,     ArrayOfUint8);
%mdv_array(int8_t,      ArrayOfInt8);
%mdv_array(uint16_t,    ArrayOfUint16);
%mdv_array(int16_t,     ArrayOfInt16);
%mdv_array(uint32_t,    ArrayOfUint32);
%mdv_array(int32_t,     ArrayOfInt32);
%mdv_array(uint64_t,    ArrayOfUint64);
%mdv_array(int64_t,     ArrayOfInt64);
%mdv_array(float,       ArrayOfFloat);
%mdv_array(double,      ArrayOfDouble);

%{
typedef struct mdv_datums
{
    uint32_t   size;
    mdv_datum *data[1];
} mdv_datums;
%}

%ignore mdv_datums::data;

%rename(Row) mdv_datums;

%nodefault;
typedef struct mdv_datums {} mdv_datums;
%clearnodefault;

%extend mdv_datums
{
    mdv_datums(uint32_t size)
    {
        mdv_datums *datums = mdv_alloc(offsetof(mdv_datums, data)
                                        + sizeof(mdv_datum*) * size,
                                       "datums");

        if(!datums)
            return 0;

        datums->size = size;

        memset(datums->data, 0, sizeof(mdv_datum*) * size);

        return datums;
    }

    ~mdv_datums()
    {
        mdv_free($self, "datums");
    }

    uint32_t cols() const
    {
        return $self->size;
    }

    bool set(size_t idx, ArrayOfBool *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_bool_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_bool_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfChar *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_char_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_char_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, char const *str)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_char_create(str, strlen(str), &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_char_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfByte *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_byte_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_byte_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfUint8 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_uint8_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_uint8_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfInt8 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_int8_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_int8_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfUint16 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_uint16_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_uint16_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfInt16 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_int16_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_int16_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfUint32 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_uint32_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_uint32_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfInt32 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_int32_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_int32_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfUint64 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_uint64_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_uint64_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfInt64 *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_int64_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_int64_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfFloat *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_float_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_float_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }

    bool set(size_t idx, ArrayOfDouble *arr, uint32_t size)
    {
        if (idx >= $self->size)
            return false;
        mdv_datum *datum = mdv_datum_double_create(arr, size, &mdv_default_allocator);
        if (!datum)
            return false;
        mdv_datum_double_free($self->data[idx]);
        $self->data[idx] = datum;
        return true;
    }
}
