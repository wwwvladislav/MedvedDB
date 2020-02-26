%module mdv

%inline %{
#include <mdv_datum.h>
%}

%include "stdint.i"
%include "exception.i"
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

#define mdv_datums_functions(T, name)                                                               \
    static bool mdv_datums_set_##name(mdv_datums *row, size_t idx, T const *arr, uint32_t size)     \
    {                                                                                               \
        if (idx >= row->size)                                                                       \
            return false;                                                                           \
        mdv_datum *datum = mdv_datum_##name##_create(arr, size);                                    \
        if (!datum)                                                                                 \
            return false;                                                                           \
        mdv_datum_free(row->data[idx]);                                                             \
        row->data[idx] = datum;                                                                     \
        return true;                                                                                \
    }

mdv_datums_functions(bool,      bool);
mdv_datums_functions(char,      char);
mdv_datums_functions(uint8_t,   byte);
mdv_datums_functions(int8_t,    int8);
mdv_datums_functions(uint8_t,   uint8);
mdv_datums_functions(int16_t,   int16);
mdv_datums_functions(uint16_t,  uint16);
mdv_datums_functions(int32_t,   int32);
mdv_datums_functions(uint32_t,  uint32);
mdv_datums_functions(int64_t,   int64);
mdv_datums_functions(uint64_t,  uint64);
mdv_datums_functions(float,     float);
mdv_datums_functions(double,    double);

#undef mdv_datums_functions
%}

%ignore mdv_datums::data;

%rename(Row) mdv_datums;

%nodefault;
typedef struct mdv_datums {} mdv_datums;
%clearnodefault;

%exception mdv_datums::getBool(size_t idx)
{
    if(arg2 >= arg1->size)
        SWIG_exception(SWIG_IndexError, "Invalid array index");
    if(mdv_datum_type(arg1->data[arg2]) != MDV_FLD_TYPE_BOOL)
        SWIG_exception(SWIG_TypeError, "Invalid cast");
    $action
}

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
        for(uint32_t i = 0; i < $self->size; ++i)
            mdv_datum_free($self->data[i]);
        mdv_free($self, "datums");
    }

    uint32_t cols() const
    {
        return $self->size;
    }

    bool getBool(size_t idx)                                            { return false; }
    bool setBool(size_t idx, bool val)                                  { return mdv_datums_set_bool($self, idx, &val, 1); }
    bool setBoolArray(size_t idx, ArrayOfBool *arr, uint32_t size)      { return mdv_datums_set_bool($self, idx, arr, size); }

    bool setChar(size_t idx, char val)                                  { return mdv_datums_set_char($self, idx, &val, 1); }
    bool setCharArray(size_t idx, ArrayOfChar *arr, uint32_t size)      { return mdv_datums_set_char($self, idx, arr, size); }
    bool setString(size_t idx, char const *str)                         { return mdv_datums_set_char($self, idx, str, strlen(str)); }

    bool setByte(size_t idx, uint8_t val)                               { return mdv_datums_set_uint8($self, idx, &val, 1); }
    bool setByteArray(size_t idx, ArrayOfByte *arr, uint32_t size)      { return mdv_datums_set_byte($self, idx, arr, size); }

    bool setUint8(size_t idx, uint8_t val)                              { return mdv_datums_set_uint8($self, idx, &val, 1); }
    bool setUint8Array(size_t idx, ArrayOfUint8 *arr, uint32_t size)    { return mdv_datums_set_uint8($self, idx, arr, size); }

    bool setInt8(size_t idx, int8_t val)                                { return mdv_datums_set_int8($self, idx, &val, 1); }
    bool setInt8Array(size_t idx, ArrayOfInt8 *arr, uint32_t size)      { return mdv_datums_set_int8($self, idx, arr, size); }

    bool setUint16(size_t idx, uint16_t val)                            { return mdv_datums_set_uint16($self, idx, &val, 1); }
    bool setUint16Array(size_t idx, ArrayOfUint16 *arr, uint32_t size)  { return mdv_datums_set_uint16($self, idx, arr, size); }

    bool setInt16(size_t idx, int16_t val)                              { return mdv_datums_set_int16($self, idx, &val, 1); }
    bool setInt16Array(size_t idx, ArrayOfInt16 *arr, uint32_t size)    { return mdv_datums_set_int16($self, idx, arr, size); }

    bool setUint32(size_t idx, uint32_t val)                            { return mdv_datums_set_uint32($self, idx, &val, 1); }
    bool setUint32Array(size_t idx, ArrayOfUint32 *arr, uint32_t size)  { return mdv_datums_set_uint32($self, idx, arr, size); }

    bool setInt32(size_t idx, int32_t val)                              { return mdv_datums_set_int32($self, idx, &val, 1); }
    bool setInt32Array(size_t idx, ArrayOfInt32 *arr, uint32_t size)    { return mdv_datums_set_int32($self, idx, arr, size); }

    bool setUint64(size_t idx, uint64_t val)                            { return mdv_datums_set_uint64($self, idx, &val, 1); }
    bool setUint64Array(size_t idx, ArrayOfUint64 *arr, uint32_t size)  { return mdv_datums_set_uint64($self, idx, arr, size); }

    bool setInt64(size_t idx, int64_t val)                              { return mdv_datums_set_int64($self, idx, &val, 1); }
    bool setInt64Array(size_t idx, ArrayOfInt64 *arr, uint32_t size)    { return mdv_datums_set_int64($self, idx, arr, size); }

    bool setFloat(size_t idx, float val)                                { return mdv_datums_set_float($self, idx, &val, 1); }
    bool setFloatArray(size_t idx, ArrayOfFloat *arr, uint32_t size)    { return mdv_datums_set_float($self, idx, arr, size); }

    bool setDouble(size_t idx, double val)                              { return mdv_datums_set_double($self, idx, &val, 1); }
    bool setDoubleArray(size_t idx, ArrayOfDouble *arr, uint32_t size)  { return mdv_datums_set_double($self, idx, arr, size); }
}
