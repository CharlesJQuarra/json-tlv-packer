
#include <apr_strings.h>

#include <json_visit.h>

#include "jp_tlv_encoder.h"

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

size_t jp_find_or_add_key(jp_TLV_records_t *records,
                          const char*       key)
{
  void* index_as_ptr = apr_hash_get(records->key_index, key, APR_HASH_KEY_STRING);

  if (NULL == index_as_ptr) {
    size_t      next_pos = apr_hash_count(records->key_index) + 1;
    apr_pool_t* pool     = apr_hash_pool_get(records->key_index);
    const char* key_copy = apr_pmemdup(pool, key, strlen(key) + 1);
    apr_hash_set(records->key_index, key_copy, APR_HASH_KEY_STRING, (void*)next_pos);

    return next_pos;
  }

  return (size_t)index_as_ptr;
}

int jp_add_boolean_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index = key_index;

  return 0;
}

int jp_add_string_kv_pair_to_record(jp_TLV_record_t *record,
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

int jp_add_integer_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value)
{
  jp_TLV_kv_pair_t* kv_pair = apr_array_push(record->kv_pairs_array);

  kv_pair->key_index = key_index;

  return 0;
}

int jp_add_double_kv_pair_to_record(jp_TLV_record_t *record,
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
  jp_TLV_record_t   *tlv_record;
  jp_TLV_records_t  *tlv_records;

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

    printf("json_visitor 0; flags: 0x%x, type: %d  key: %s, index: %ld, value: %s\n", flags, type,
	       (jso_key ? jso_key : "(null)"), (jso_index ? (long)*jso_index : -1L),
	       json_object_to_json_string(jso));

    if (flags == JSON_C_VISIT_SECOND || parent_jso != builder->jso || jso_key == NULL)
      return JSON_C_VISIT_RETURN_CONTINUE;

    size_t key_index = jp_find_or_add_key(builder->tlv_records, jso_key);

    printf("json_visitor 1; flags: 0x%x, type: %d  key: %s, index of key: %ld, value: %s\n", flags, type,
	       (jso_key ? jso_key : "(null)"), key_index, json_object_to_json_string(jso));

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
                                jp_TLV_records_t *records,
                                json_object      *jso)
{
  jp_TLV_record_builder_t builder;

  builder.jso         = jso;
  builder.tlv_record  = jp_TLV_record_make(pool);
  builder.tlv_records = records;

  json_c_visit(jso, 0, json_record_builder_visitor, &builder);

  return 0;
}

int jp_export_record_to_file(jp_TLV_record_t *record,
                             FILE            *output)
{
}

int jp_export_key_array_to_file(apr_hash_t *key_index,
                                FILE       *output)
{
}

static int hash_do_iterate(void *rec, const void *key, apr_ssize_t klen, const void *value)
{
  apr_hash_t* index_key = rec;
  printf("hash_do_iterate: key %s -> val %ld \n", (const char*)key, (size_t)value);
  apr_hash_set(index_key, value, sizeof(size_t), key);
  return 1;
}

static unsigned int index_hash(const char *key, apr_ssize_t *klen)
{
  return (unsigned int)key;
}

apr_hash_t* jp_build_index_2_key_from_key_index(apr_hash_t* key_index)
{
  apr_hash_t* index_key = apr_hash_make_custom(apr_hash_pool_get(key_index), index_hash);

  apr_hash_do(hash_do_iterate, index_key, key_index);

  return index_key;
}