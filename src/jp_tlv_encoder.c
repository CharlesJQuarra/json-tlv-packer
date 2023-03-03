
#include <apr_strings.h>

#include "jp_tlv_encoder.h"

size_t find_or_add_key(jp_TLV_records_t *records,
                       const char*       key)
{
  void* index_as_ptr = apr_hash_get(records->key_index, key, APR_HASH_KEY_STRING);

  if (NULL == index_as_ptr) {
    size_t next_pos = apr_hash_count(records->key_index) + 1;
    apr_hash_set(records->key_index, key, APR_HASH_KEY_STRING, next_pos);

    return next_pos;
  }

  return index_as_ptr;
}

int add_boolean_kv_pair_to_record(jp_TLV_record_t *record,
                                  size_t           key_index,
                                  int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index = key_index;

  return 0;
}

int add_string_kv_pair_to_record(jp_TLV_record_t *record,
                                 size_t           key_index,
                                 const char*      value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);
  apr_pool_t*       pool    = record->kv_pairs_array->pool;

  kv_pair->key_index    = key_index;
  kv_pair->value_buffer = apr_pmemdup(pool, value, strlen(value) + 1);
  kv_pair->value_length = strlen(value);

  return 0;
}

int add_integer_kv_pair_to_record(jp_TLV_record_t *record,
                                  size_t           key_index,
                                  int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index = key_index;

  return 0;
}

int add_double_kv_pair_to_record(jp_TLV_record_t *record,
                                 size_t           key_index,
                                 double           value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index = key_index;

  return 0;
}

typedef struct jp_TLV_record_builder
{

  const json_object *jso;
  jp_TLV_record     *tlv_record;
  jp_TLV_records_t  *tlv_records;

} jp_TLV_record_builder_t;

static int json_visitor(json_object *jso,
                        int          flags,
                        json_object *parent_jso,
                        const char  *jso_key,
                        size_t      *jso_index,
                        void        *userarg)
{
    jp_TLV_record_builder* builder = userarg;

    if (flags == JSON_C_VISIT_SECOND || parent_jso != builder->jso || jso_key == NULL)
      return JSON_C_VISIT_RETURN_CONTINUE;

    enum json_type type = json_object_get_type(jso);
    size_t key_index    = find_or_add_key(builder->tlv_records, jso_key);


    printf("flags: 0x%x, type: %d  key: %s, index of key: %ld, value: %s\n", flags, type,
	       (jso_key ? jso_key : "(null)"), key_index, json_object_to_json_string(jso));

    switch (type) {

      case json_type_boolean:
      add_boolean_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_boolean(jso));
      break;
  	  case json_type_double:
      add_double_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_double(jso));
      break;
      case json_type_int:
      add_integer_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_int(jso));
      break;
  	  case json_type_string:
      add_string_kv_pair_to_record(builder->tlv_record, key_index, json_object_get_string(jso));
      break;

      default:
    }

    return JSON_C_VISIT_RETURN_CONTINUE;
}

jp_TLV_record_t* jp_TLV_record_make(apr_pool_t *pool)
{
  jp_TLV_record_t* record = apr_palloc(pool, sizeof(jp_TLV_record_t) );

  record->kv_pairs_array = apr_array_make(pool, 1, sizeof(jp_TLV_kv_pair_t));

  return record;
}

int jp_update_records_from_json(apr_pool_t        *pool,
                                jp_TLV_records    *records,
                                const json_object *jso)
{
  jp_TLV_record_builder builder;

  builder.jso         = jso;
  builder.tlv_record  = jp_TLV_record_make(pool);
  builder.tlv_records = records;

  json_c_visit(jso, 0, json_visitor, &builder);

}

int jp_export_record_to_file(jp_TLV_record *record,
                             FILE          *output)
{
}

int jp_export_key_array_to_file(apr_array_header_t *key_array,
                                FILE               *output)
{
}