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

%define %mdv_datum_getter_exceptions(GETTER, FLD_TYPE)
    %exception mdv_datums::GETTER(size_t idx)
    {
        if(arg2 >= arg1->size)
            SWIG_exception(SWIG_IndexError, "Invalid array index");
        if(mdv_datum_type(arg1->data[arg2]) != FLD_TYPE)
            SWIG_exception(SWIG_TypeError, "Invalid cast");
        $action
    }
%enddef

%mdv_datum_getter_exceptions(getBool,        MDV_FLD_TYPE_BOOL);
%mdv_datum_getter_exceptions(getBoolArray,   MDV_FLD_TYPE_BOOL);
%mdv_datum_getter_exceptions(getChar,        MDV_FLD_TYPE_CHAR);
%mdv_datum_getter_exceptions(getCharArray,   MDV_FLD_TYPE_CHAR);
%mdv_datum_getter_exceptions(getString,      MDV_FLD_TYPE_CHAR);
%mdv_datum_getter_exceptions(getByte,        MDV_FLD_TYPE_BYTE);
%mdv_datum_getter_exceptions(getByteArray,   MDV_FLD_TYPE_BYTE);
%mdv_datum_getter_exceptions(getUint8,       MDV_FLD_TYPE_UINT8);
%mdv_datum_getter_exceptions(getUint8Array,  MDV_FLD_TYPE_UINT8);
%mdv_datum_getter_exceptions(getInt8,        MDV_FLD_TYPE_INT8);
%mdv_datum_getter_exceptions(getInt8Array,   MDV_FLD_TYPE_INT8);
%mdv_datum_getter_exceptions(getUint16,      MDV_FLD_TYPE_UINT16);
%mdv_datum_getter_exceptions(getUint16Array, MDV_FLD_TYPE_UINT16);
%mdv_datum_getter_exceptions(getInt16,       MDV_FLD_TYPE_INT16);
%mdv_datum_getter_exceptions(getInt16Array,  MDV_FLD_TYPE_INT16);
%mdv_datum_getter_exceptions(getUint32,      MDV_FLD_TYPE_UINT32);
%mdv_datum_getter_exceptions(getUint32Array, MDV_FLD_TYPE_UINT32);
%mdv_datum_getter_exceptions(getInt32,       MDV_FLD_TYPE_INT32);
%mdv_datum_getter_exceptions(getInt32Array,  MDV_FLD_TYPE_INT32);
%mdv_datum_getter_exceptions(getUint64,      MDV_FLD_TYPE_UINT64);
%mdv_datum_getter_exceptions(getUint64Array, MDV_FLD_TYPE_UINT64);
%mdv_datum_getter_exceptions(getInt64,       MDV_FLD_TYPE_INT64);
%mdv_datum_getter_exceptions(getInt64Array,  MDV_FLD_TYPE_INT64);
%mdv_datum_getter_exceptions(getFloat,       MDV_FLD_TYPE_FLOAT);
%mdv_datum_getter_exceptions(getFloatArray,  MDV_FLD_TYPE_FLOAT);
%mdv_datum_getter_exceptions(getDouble,      MDV_FLD_TYPE_DOUBLE);
%mdv_datum_getter_exceptions(getDoubleArray, MDV_FLD_TYPE_DOUBLE);

%exception mdv_datums::fieldSize(size_t idx)
{
    if(arg2 >= arg1->size)
        SWIG_exception(SWIG_IndexError, "Invalid array index");
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

    uint32_t fieldSize(size_t idx)
    {
        return mdv_datum_count($self->data[idx]);
    }

    mdv_field_type fieldType(size_t idx)
    {
        return mdv_datum_type($self->data[idx]);
    }

    bool                  getBool(size_t idx)                           { return *mdv_datum_as_bool($self->data[idx]); }
    ArrayOfBool const *   getBoolArray(size_t idx)                      { return mdv_datum_as_bool($self->data[idx]); }
    char                  getChar(size_t idx)                           { return *mdv_datum_as_char($self->data[idx]); }
    ArrayOfChar const *   getCharArray(size_t idx)                      { return mdv_datum_as_char($self->data[idx]); }
    char const *          getString(size_t idx)                         { return mdv_datum_as_char($self->data[idx]); }
    uint8_t               getByte(size_t idx)                           { return *mdv_datum_as_byte($self->data[idx]); }
    ArrayOfByte const *   getByteArray(size_t idx)                      { return mdv_datum_as_byte($self->data[idx]); }
    uint8_t               getUint8(size_t idx)                          { return *mdv_datum_as_uint8($self->data[idx]); }
    ArrayOfUint8 const *  getUint8Array(size_t idx)                     { return mdv_datum_as_uint8($self->data[idx]); }
    int8_t                getInt8(size_t idx)                           { return *mdv_datum_as_int8($self->data[idx]); }
    ArrayOfInt8 const *   getInt8Array(size_t idx)                      { return mdv_datum_as_int8($self->data[idx]); }
    uint16_t              getUint16(size_t idx)                         { return *mdv_datum_as_uint16($self->data[idx]); }
    ArrayOfUint16 const * getUint16Array(size_t idx)                    { return mdv_datum_as_uint16($self->data[idx]); }
    int16_t               getInt16(size_t idx)                          { return *mdv_datum_as_int16($self->data[idx]); }
    ArrayOfInt16 const *  getInt16Array(size_t idx)                     { return mdv_datum_as_int16($self->data[idx]); }
    uint32_t              getUint32(size_t idx)                         { return *mdv_datum_as_uint32($self->data[idx]); }
    ArrayOfUint32 const * getUint32Array(size_t idx)                    { return mdv_datum_as_uint32($self->data[idx]); }
    int32_t               getInt32(size_t idx)                          { return *mdv_datum_as_int32($self->data[idx]); }
    ArrayOfInt32 const *  getInt32Array(size_t idx)                     { return mdv_datum_as_int32($self->data[idx]); }
    uint64_t              getUint64(size_t idx)                         { return *mdv_datum_as_uint64($self->data[idx]); }
    ArrayOfUint64 const * getUint64Array(size_t idx)                    { return mdv_datum_as_uint64($self->data[idx]); }
    int64_t               getInt64(size_t idx)                          { return *mdv_datum_as_int64($self->data[idx]); }
    ArrayOfInt64 const *  getInt64Array(size_t idx)                     { return mdv_datum_as_int64($self->data[idx]); }
    float                 getFloat(size_t idx)                          { return *mdv_datum_as_float($self->data[idx]); }
    ArrayOfFloat const *  getFloatArray(size_t idx)                     { return mdv_datum_as_float($self->data[idx]); }
    double                getDouble(size_t idx)                         { return *mdv_datum_as_double($self->data[idx]); }
    ArrayOfDouble const * getDoubleArray(size_t idx)                    { return mdv_datum_as_double($self->data[idx]); }

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
