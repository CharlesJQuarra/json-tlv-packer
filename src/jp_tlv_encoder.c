
#include <apr_strings.h>

#include <json_visit.h>

#include <jp_tlv_encoder.h>
#include "jp_tlv_encoder_private.h"

#include <assert.h>
#include <math.h>


jp_TLV_records_t* jp_TLV_record_collection_make(apr_pool_t *pool)
{
  jp_TLV_records_t* record_collection = apr_palloc(pool, sizeof(jp_TLV_records_t) );

  record_collection->key_index   = apr_hash_make(pool);
  record_collection->record_list = apr_array_make(pool, 1, sizeof(jp_TLV_record_t*));

  return record_collection;
}


jp_TLV_record_t* jp_TLV_record_make(apr_pool_t *pool)
{
  jp_TLV_record_t* record = apr_palloc(pool, sizeof(jp_TLV_record_t) );

  record->kv_pairs_array = apr_array_make(pool, 1, sizeof(jp_TLV_kv_pair_t));

  return record;
}


int jp_add_record_to_TLV_collection(jp_TLV_records_t *record_collection,
                                    jp_TLV_record_t  *record)
{
  jp_TLV_record_t** entry = apr_array_push(record_collection->record_list);

  *entry = record;

  return 0;
}


size_t jp_find_or_add_key(apr_hash_t       *key_index,
                          const char*       key)
{
  void* index_as_ptr = apr_hash_get(key_index, key, APR_HASH_KEY_STRING);

  if (NULL == index_as_ptr) {
    size_t      next_pos = apr_hash_count(key_index) + 1;
    apr_pool_t* pool     = apr_hash_pool_get(key_index);
    const char* key_copy = apr_pmemdup(pool, key, strlen(key) + 1);
    apr_hash_set(key_index, key_copy, APR_HASH_KEY_STRING, (void*)next_pos);

    return next_pos;
  }

  return (size_t)index_as_ptr;
}

uint32_t jp_export_uint32_to_buffer(uint32_t        value,
                                    jp_buffer_io_t *buffer)
{
  if (jp_buffer_io_bytes_left_to_write(buffer) < sizeof(uint32_t))
    jp_buffer_io_flush_writes(buffer);

  jp_buffer_io_memcpy_to(buffer, & value, sizeof(uint32_t));
  return sizeof(uint32_t);
}

uint32_t jp_import_uint32_from_buffer(uint32_t       *value,
                                      jp_buffer_io_t *buffer)
{
  if (jp_buffer_io_bytes_left_to_read(buffer) < sizeof(uint32_t)) {
    if (0 != jp_buffer_io_read(buffer)) {
      return 0;
    }
  }

  jp_buffer_io_memcpy_from(buffer, value, sizeof(uint32_t));
  return sizeof(uint32_t);
}

/*
 *  In the current implementation:
 *
 *  a kv pair record must at least have the first descriptor byte,
 *  which consists of:
 *
 *  - 2 type bits for describing the field type: boolean, integer, string or double
 *  - 1 'will fit' bit for declaring if the value will fit in the next 5 bits (boolean or
 *    integers below 32, or the length of strings shorter than 32 characters)
 *  - 5 extra bits for value or length use if the 3rd bit is set
 *
 *  if the type is integer and the 3rd bit is unset, then the record takes 4 extra bytes to store the value
 *
 *  if the type is double and __STDC_IEC_559__ is defined, the record takes 8 extra bytes to store the value
 *  regardless of the 3rd bit, if __STDC_IEC_559__ is not defined we should produce a compilation error as this
 *  case is unaddressed in this implementation
 *
 *  if the type is string, and the 3rd bit is unset, then the record takes 4 extra bytes to store the length
 *  interpreted as an uint32_t, and as much extra bytes as the stored length specifies
 *
 *  if the type is string and the 3rd bit is set, the the length is stored in the 5 extra bits of the 1st byte,
 *  and as much extra bytes as this length specifies (31 characters or less)
 *
 *  if the type is boolean, the 3rd bit is ignored, since a boolean value will always fit in 5 bits.
 *
 */

#ifndef __STDC_IEC_559__
    #error __STDC_IEC_559__ is not defined for this architecture
#endif

#define TYPE_MASK         (((uint8_t) 3 ) << 6)
#define TYPE_MASK_C       ~(((uint8_t) 3 ) << 6)
#define TYPE_SHIFT        (6)

#define WILL_FIT_MASK     (((uint8_t) 1 ) << 5)
#define WILL_FIT_MASK_C   ~(((uint8_t) 1 ) << 5)
#define WILL_FIT_SHIFT    (5)

#define EXTRA_BITS_MASK   ((uint8_t) 31)
#define EXTRA_BITS_MASK_C ~((uint8_t) 31)

static inline
uint8_t
get_type_bits(uint8_t byte)
{
  return (byte & TYPE_MASK) >> TYPE_SHIFT;
}

static inline
void
set_type_bits(uint8_t* byte, uint32_t value)
{
  *byte = (*byte & TYPE_MASK_C) | ((value << TYPE_SHIFT) & TYPE_MASK);
}

static inline
uint8_t
get_will_fit_bit(uint8_t byte)
{
  return byte & WILL_FIT_MASK;
}

static inline
void
set_will_fit_bit(uint8_t* byte, uint32_t value)
{
  *byte = (*byte & WILL_FIT_MASK_C) | ((value << WILL_FIT_SHIFT) & WILL_FIT_MASK);
}

static inline
uint8_t
get_extra_bits(uint8_t byte)
{
  return byte & EXTRA_BITS_MASK;
}

static inline
void
set_extra_bits(uint8_t* byte, uint32_t value)
{
  *byte = (*byte & EXTRA_BITS_MASK_C) | (value & EXTRA_BITS_MASK);
}

#undef TYPE_MASK
#undef TYPE_MASK_C
#undef TYPE_SHIFT

#undef WILL_FIT_MASK
#undef WILL_FIT_MASK_C
#undef WILL_FIT_SHIFT

uint32_t jp_export_value_union_to_buffer(const jp_TLV_union_t *union_value,
                                               uint32_t        value_type,
                                               jp_buffer_io_t *buffer)
{
  if (jp_buffer_io_bytes_left_to_write(buffer) < 1)
    jp_buffer_io_flush_writes(buffer);

  uint8_t descriptor_byte = 0;
  set_type_bits(& descriptor_byte, value_type);

        uint32_t         written  = 1;
        uint32_t         required = 1;
        int32_t          integer_value;
        double           double_value;
  const jp_TLV_string_t* string_value;
        uint32_t         will_fit;

  switch(value_type) {
    case JP_TYPE_BOOLEAN:

    set_extra_bits(& descriptor_byte, !! union_value->integer_value);
    jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));

    break;

    case JP_TYPE_INTEGER:

    integer_value = union_value->integer_value;
    will_fit      = (((int32_t)(integer_value & EXTRA_BITS_MASK)) == integer_value);
    set_will_fit_bit(& descriptor_byte, will_fit);

    if (will_fit) {
      set_extra_bits(& descriptor_byte, integer_value);
      jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));
    } else {
      jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));

      required += sizeof(int32_t);

      if (jp_buffer_io_bytes_left_to_write(buffer) < sizeof(int32_t))
        jp_buffer_io_flush_writes(buffer);

      jp_buffer_io_memcpy_to(buffer, & integer_value, sizeof(int32_t));
      written += sizeof(int32_t);
    }

    break;

    case JP_TYPE_DOUBLE:

    jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));

    required    += sizeof(double);
    double_value = union_value->double_value;

    if (jp_buffer_io_bytes_left_to_write(buffer) < sizeof(double))
      jp_buffer_io_flush_writes(buffer);

    jp_buffer_io_memcpy_to(buffer, & double_value, sizeof(double));
    written += sizeof(double);

    break;

    case JP_TYPE_STRING:

    string_value = & union_value->string_value;
    required    += string_value->value_length;
    will_fit     = (((uint32_t)(string_value->value_length & EXTRA_BITS_MASK)) == string_value->value_length);
    set_will_fit_bit(&descriptor_byte, will_fit);

    if (will_fit) {
      set_extra_bits(& descriptor_byte, string_value->value_length);
      jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));
    } else {
      jp_buffer_io_memcpy_to(buffer, & descriptor_byte, sizeof(uint8_t));

      required += sizeof(uint32_t);

      if (jp_buffer_io_bytes_left_to_write(buffer) < sizeof(uint32_t))
        jp_buffer_io_flush_writes(buffer);

      jp_buffer_io_memcpy_to(buffer, & string_value->value_length, sizeof(uint32_t));
      written += sizeof(uint32_t);
    }

    if (buffer->current_size < string_value->value_length) {
      if (0 != jp_buffer_io_grow(buffer, string_value->value_length)) {
        written = 0;
        break;
      }
    }

    if (jp_buffer_io_bytes_left_to_write(buffer) < string_value->value_length)
      jp_buffer_io_flush_writes(buffer);

    jp_buffer_io_memcpy_to(buffer, string_value->value_buffer, string_value->value_length);
    written += string_value->value_length;

    break;

    default:
    written = 0;
    break;
  }

  return written;
}

uint32_t jp_import_value_union_from_buffer(apr_pool_t     *pool,
                                           jp_TLV_union_t *union_value,
                                           uint32_t       *value_type,
                                           jp_buffer_io_t *buffer)
{
  if (jp_buffer_io_bytes_left_to_read(buffer) < 1)
    jp_buffer_io_read(buffer);

  uint8_t descriptor_byte;
  jp_buffer_io_memcpy_from(buffer, & descriptor_byte, 1);

  uint32_t read     = 1;
  uint32_t required = 1;

  *value_type = get_type_bits(descriptor_byte);

  jp_TLV_string_t* string_value;

  switch (*value_type) {
    case JP_TYPE_BOOLEAN:

    union_value->integer_value = !!get_extra_bits(descriptor_byte);

    break;

    case JP_TYPE_INTEGER:

    if (get_will_fit_bit(descriptor_byte))
      union_value->integer_value = get_extra_bits(descriptor_byte);
    else
    {
      required += sizeof(int32_t);

      if (jp_buffer_io_bytes_left_to_read(buffer) < sizeof(int32_t))
        jp_buffer_io_read(buffer);

      //memcpy(& union_value->integer_value, buffer + read, sizeof(int32_t));
      jp_buffer_io_memcpy_from(buffer, & union_value->integer_value, sizeof(int32_t));
      read += sizeof(int32_t);
    }

    break;

    case JP_TYPE_DOUBLE:

    required += sizeof(double);

    if (jp_buffer_io_bytes_left_to_read(buffer) < sizeof(double))
      jp_buffer_io_read(buffer);

    //memcpy(& union_value->double_value, buffer + read, sizeof(double));
    jp_buffer_io_memcpy_from(buffer, & union_value->double_value, sizeof(double));
    read += sizeof(double);

    break;

    case JP_TYPE_STRING:

    string_value = & union_value->string_value;

    if (get_will_fit_bit(descriptor_byte)) {
      string_value->value_length = get_extra_bits(descriptor_byte);
    } else {
      required += sizeof(uint32_t);

      if (jp_buffer_io_bytes_left_to_read(buffer) < sizeof(uint32_t))
        jp_buffer_io_read(buffer);

      //memcpy(& string_value->value_length, buffer + read, sizeof(uint32_t));
      jp_buffer_io_memcpy_from(buffer, & string_value->value_length, sizeof(uint32_t));
      read += sizeof(uint32_t);
    }

    required += string_value->value_length;

    if (buffer->current_size < string_value->value_length) {
      if (0 != jp_buffer_io_grow(buffer, string_value->value_length)) {
        read = 0;
        break;
      }
    }

    if (jp_buffer_io_bytes_left_to_read(buffer) < string_value->value_length)
        jp_buffer_io_read(buffer);

    string_value->value_buffer = apr_pmemdup(pool, jp_buffer_io_use_available_bytes(buffer, string_value->value_length), string_value->value_length + 1);
    string_value->value_buffer[string_value->value_length] = '\0';
    read += string_value->value_length;

    break;

    default:
    read = 0;
    break;
  }

  return read;
}

#undef EXTRA_BITS_MASK
#undef EXTRA_BITS_MASK_C


int jp_add_boolean_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  memset(kv_pair, 0, sizeof(jp_TLV_kv_pair_t));

  kv_pair->key_index  = key_index;
  kv_pair->value_type = JP_TYPE_BOOLEAN;

  kv_pair->union_v.integer_value = !!value;

  return 0;
}

int jp_add_string_kv_pair_to_record(jp_TLV_record_t *record,
                                    size_t           key_index,
                                    const char*      value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);
  apr_pool_t*       pool    = record->kv_pairs_array->pool;

  kv_pair->key_index  = key_index;
  kv_pair->value_type = JP_TYPE_STRING;

  jp_TLV_string_t* string_value = & kv_pair->union_v.string_value;
  string_value->value_buffer    = apr_pmemdup(pool, value, strlen(value) + 1);
  string_value->value_length    = strlen(value);

  return 0;
}

int jp_add_integer_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index  = key_index;
  kv_pair->value_type = JP_TYPE_INTEGER;

  kv_pair->union_v.integer_value = value;

  return 0;
}


int jp_add_double_kv_pair_to_record(jp_TLV_record_t *record,
                                    size_t           key_index,
                                    double           value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index  = key_index;
  kv_pair->value_type = JP_TYPE_DOUBLE;

  kv_pair->union_v.double_value = value;

  return 0;
}


int jp_read_boolean_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                       int              *value)
{
  if (kv_pair->value_type != JP_TYPE_BOOLEAN)
    return -1;

  *value = !! kv_pair->union_v.integer_value;

  return 0;
}

int jp_read_string_from_kv_pair(const jp_TLV_kv_pair_t  *kv_pair,
                                      char             **value)
{
  if (kv_pair->value_type != JP_TYPE_STRING)
    return -1;

  *value = kv_pair->union_v.string_value.value_buffer;

  return 0;
}

int jp_read_integer_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                       int32_t          *value)
{
  if (kv_pair->value_type != JP_TYPE_INTEGER)
    return -1;

  *value = kv_pair->union_v.integer_value;

  return 0;
}

int jp_read_double_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                      double           *value)
{
  if (kv_pair->value_type != JP_TYPE_DOUBLE)
    return -1;

  *value = kv_pair->union_v.double_value;

  return 0;
}


uint32_t jp_export_kv_pair_to_buffer(jp_TLV_kv_pair_t *kv_pair,
                                     jp_buffer_io_t   *buffer)
{
  uint32_t k_written = jp_export_uint32_to_buffer(kv_pair->key_index, buffer);

  if (0 == k_written)
    return 0;

  uint32_t v_written = jp_export_value_union_to_buffer(& kv_pair->union_v, kv_pair->value_type, buffer);

  if (0 == v_written)
    return 0;

  return k_written + v_written;
}

uint32_t jp_export_kv_pair_to_static_buffer(jp_TLV_kv_pair_t *kv_pair,
                                            uint8_t          *buffer,
                                            size_t            buffer_size)
{
  jp_buffer_io_t buffer_io;
  jp_buffer_io_initialize_static(& buffer_io, buffer, buffer_size);

  return jp_export_kv_pair_to_buffer(kv_pair, & buffer_io);
}

uint32_t jp_import_kv_pair_from_buffer(apr_pool_t       *pool,
                                       jp_TLV_kv_pair_t *kv_pair,
                                       jp_buffer_io_t   *buffer)
{
  uint32_t k_read = jp_import_uint32_from_buffer(& kv_pair->key_index, buffer);

  if (0 == k_read)
    return 0;

  uint32_t v_read = jp_import_value_union_from_buffer(pool, & kv_pair->union_v, & kv_pair->value_type, buffer);

  if (0 == v_read)
    return 0;

  return k_read + v_read;
}

uint32_t jp_import_kv_pair_from_static_buffer(apr_pool_t       *pool,
                                              jp_TLV_kv_pair_t *kv_pair,
                                              uint8_t          *buffer,
                                              size_t            buffer_size)
{
  jp_buffer_io_t buffer_io;
  jp_buffer_io_initialize_static(& buffer_io, buffer, buffer_size);

  return jp_import_kv_pair_from_buffer(pool, kv_pair, & buffer_io);
}


uint32_t jp_export_record_to_buffer(const jp_TLV_record_t *record,
                                          jp_buffer_io_t  *buffer)
{
  uint32_t            written  = 0;
  apr_array_header_t* kv_array = record->kv_pairs_array;
  uint32_t            nb_pairs = kv_array->nelts;

  uint32_t length_written = jp_export_uint32_to_buffer(nb_pairs, buffer);

  if (0 == length_written)
    return 0;

  written += length_written;

  for (int i = 0; i < nb_pairs; i++) {
    jp_TLV_kv_pair_t* elem = & ((jp_TLV_kv_pair_t*)kv_array->elts)[i];

    uint32_t new_written = jp_export_kv_pair_to_buffer(elem, buffer);

    if (0 == new_written)
      return 0;

    written += new_written;
  }

  return written;
}

uint32_t jp_export_record_to_static_buffer(const jp_TLV_record_t *record,
                                                 uint8_t         *buffer,
                                                 size_t           buffer_size)
{
  jp_buffer_io_t buffer_io;
  jp_buffer_io_initialize_static(& buffer_io, buffer, buffer_size);

  return jp_export_record_to_buffer(record, & buffer_io);
}

uint32_t jp_import_record_from_buffer(apr_pool_t       *pool,
                                      jp_TLV_record_t **record,
                                      jp_buffer_io_t   *buffer)
{
  uint32_t read = 0;
  uint32_t nb_pairs;

  *record = jp_TLV_record_make(pool);

  apr_array_header_t* kv_array = (*record)->kv_pairs_array;

  uint32_t length_read = jp_import_uint32_from_buffer(& nb_pairs, buffer);

  if (0 == length_read)
    return 0;

  read += length_read;

  for (int i = 0; i < nb_pairs; i++) {
    jp_TLV_kv_pair_t* elem = apr_array_push(kv_array);

    uint32_t new_read = jp_import_kv_pair_from_buffer(pool, elem, buffer);

    if (0 == new_read)
      return 0;

    read += new_read;
  }

  return read;
}

uint32_t jp_import_record_from_static_buffer(apr_pool_t       *pool,
                                             jp_TLV_record_t **record,
                                             uint8_t          *buffer,
                                             size_t            buffer_size)
{
  jp_buffer_io_t buffer_io;
  jp_buffer_io_initialize_static(& buffer_io, buffer, buffer_size);

  return jp_import_record_from_buffer(pool, record, & buffer_io);
}

typedef struct jp_TLV_record_builder
{

  const json_object *jso;
  jp_TLV_record_t   *tlv_record;
  apr_hash_t        *key_index;

} jp_TLV_record_builder_t;


static int json_record_builder_visitor(json_object *jso,
                                       int          flags,
                                       json_object *parent_jso,
                                       const char  *jso_key,
                                       size_t      *jso_index,
                                       void        *userarg)
{
    jp_TLV_record_builder_t* builder = userarg;
    enum json_type           type    = json_object_get_type(jso);

    if (flags == JSON_C_VISIT_SECOND || parent_jso != builder->jso || jso_key == NULL)
      return JSON_C_VISIT_RETURN_CONTINUE;

    size_t key_index = jp_find_or_add_key(builder->key_index, jso_key);

    switch (type) {

      case json_type_boolean:
      jp_add_boolean_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_boolean(jso));
      break;
  	  case json_type_double:
      jp_add_double_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_double(jso));
      break;
      case json_type_int:
      jp_add_integer_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_int(jso));
      break;
  	  case json_type_string:
      jp_add_string_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_string(jso));
      break;

      default:
      break;
    }

    return JSON_C_VISIT_RETURN_CONTINUE;
}


int jp_update_records_from_json(apr_pool_t       *pool,
                                jp_TLV_records_t *record_collection,
                                json_object      *jso)
{
  jp_TLV_record_builder_t builder;

  builder.jso         = jso;
  builder.tlv_record  = jp_TLV_record_make(pool);
  builder.key_index   = record_collection->key_index;

  json_c_visit(jso, 0, json_record_builder_visitor, & builder);
  jp_add_record_to_TLV_collection(record_collection, builder.tlv_record);

  return 0;
}


