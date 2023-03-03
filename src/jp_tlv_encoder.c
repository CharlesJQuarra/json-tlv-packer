
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

#define JP_IO_HELPER_BUFFER_SIZE 4096
typedef struct jp_buffer_io_helper
{
  size_t used;
  char   buffer[JP_IO_HELPER_BUFFER_SIZE];
  int   eof;
  FILE  *stream;

} jp_buffer_io_helper_t;

#include <inttypes.h>

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

static int jp_buffer_write_key_index_pair(void *rec, const void *key, apr_ssize_t klen, const void *value)
{
  jp_buffer_io_helper_t* helper = rec;

  printf("jp_buffer_write_key_index_pair: key %s klen %ld -> val %ld \n", (const char*)key, klen, (size_t)value);
  uint32_t expected_length_to_write = sizeof(uint32_t) + sizeof(uint32_t) + (size_t)klen;

  if (JP_IO_HELPER_BUFFER_SIZE < expected_length_to_write)
    return 0;

  printf("jp_buffer_write_key_index_pair: expected_length_to_write: %d \n", expected_length_to_write);

  if (helper->used + expected_length_to_write > JP_IO_HELPER_BUFFER_SIZE) {
    fwrite(helper->buffer, 1, helper->used, helper->stream);
    helper->used = 0;
  }

  memcpy(helper->buffer + helper->used, &expected_length_to_write, sizeof(uint32_t));
  expected_length_to_write -= sizeof(uint32_t);
  helper->used             += sizeof(uint32_t);

  //printf("b used: %ld buffer point at: 0x%.12" PRIXPTR "\n", writer_helper->used, (uintptr_t)writer_helper->buffer);
  //printf("attempting a copy at: 0x%.12" PRIXPTR "\n", (uintptr_t)(writer_helper->buffer + writer_helper->used));

  memcpy(helper->buffer + helper->used, &value, sizeof(uint32_t));
  expected_length_to_write -= sizeof(uint32_t);
  helper->used             += sizeof(uint32_t);
  printf("jp_buffer_write_key_index_pair: index value: %d \n", (uint32_t)value);

  memcpy(helper->buffer + helper->used, key, expected_length_to_write);
  helper->used += expected_length_to_write;

  printf("jp_buffer_write_key_index_pair: expected_length_to_write to end of segment: %d \n", expected_length_to_write);

  for (int i = 0; i < helper->used; ++i) {
    //printf("0b%08B", writer_helper->buffer[i]);
    printf(":"BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(helper->buffer[i]));
  }
  printf("\n");

  return 1;
}

int jp_export_key_array_to_file(apr_hash_t *key_index,
                                FILE       *output)
{
  jp_buffer_io_helper_t helper;

  helper.used   = 0;
  helper.stream = output;
  memset(helper.buffer, 0, JP_IO_HELPER_BUFFER_SIZE);

  uint64_t length = apr_hash_count(key_index);

  memcpy(helper.buffer, &length, sizeof(uint64_t));
  helper.used += sizeof(uint64_t);

  int ret = !apr_hash_do(jp_buffer_write_key_index_pair, &helper, key_index);

  /* write any leftover buffered data */
  if (helper.used > 0)
    fwrite(helper.buffer, 1, helper.used, output);

  printf("jp_export_key_array_to_file: ret %d \n", ret);
  return ret;
}

static int jp_read_io_helper(jp_buffer_io_helper_t* helper, size_t ahead, size_t* read)
{
  printf("jp_read_io_helper: helper->used: %ld ahead: %ld read: %ld \n", helper->used, ahead, *read);

  if (helper->used + ahead > *read) {
    int leftover = *read - helper->used;

    printf("jp_read_io_helper: leftover %d \n", leftover);

    if (leftover < 0)
      *read = fread(helper->buffer, 1, JP_IO_HELPER_BUFFER_SIZE, helper->stream);
    else if (leftover > 0) {
      memmove(helper->buffer, helper->buffer + helper->used, leftover);
      *read = fread(helper->buffer + leftover, 1, JP_IO_HELPER_BUFFER_SIZE - leftover, helper->stream);
    }

    helper->eof  = feof(helper->stream);
    helper->used = 0;
  }

  return 0;
}

apr_hash_t* jp_import_TLV_key_index_from_file(apr_pool_t *pool,
                                              FILE       *input)
{
  jp_buffer_io_helper_t helper;

  helper.used   = 0;
  helper.stream = input;
  memset(helper.buffer, 0, JP_IO_HELPER_BUFFER_SIZE);

  uint64_t length;
  size_t read = fread(&length, 1, sizeof(uint64_t), input);
  helper.eof  = feof(input);
  if (read < sizeof(uint64_t))
    return NULL;

  printf("jp_import_TLV_key_index_from_file: pending_pairs_to_read: %ld \n", length);

  read       = fread(helper.buffer, 1, JP_IO_HELPER_BUFFER_SIZE, input);
  helper.eof = feof(input);

  apr_hash_t* key_index = apr_hash_make(pool);
  uint64_t pending_pairs_to_read = length;

  while(pending_pairs_to_read > 0) {

    if (jp_read_io_helper(&helper, sizeof(uint32_t), &read))
      return NULL;

    uint32_t expected_length_to_write;
    memcpy(&expected_length_to_write, helper.buffer + helper.used, sizeof(uint32_t));

    printf("jp_import_TLV_key_index_from_file: expected_length_to_write: %d \n", expected_length_to_write);
    expected_length_to_write -= sizeof(uint32_t);
    helper.used              += sizeof(uint32_t);

    if (jp_read_io_helper(&helper, expected_length_to_write, &read))
      return NULL;

    uint32_t index_value;
    memcpy(&index_value, helper.buffer + helper.used, sizeof(uint32_t));
    expected_length_to_write -= sizeof(uint32_t);
    helper.used              += sizeof(uint32_t);

    //printf("jp_import_TLV_key_index_from_file: index_value: %d expected_length_to_write to end of segment %d  \n", index_value, expected_length_to_write);

    char* key    = apr_pmemdup(pool, helper.buffer + helper.used, expected_length_to_write + 1);
    char zero    = 0;
    helper.used += expected_length_to_write;
    memcpy(key + expected_length_to_write, &zero, 1);

    //printf("jp_import_TLV_key_index_from_file: key: %s \n", key);

    apr_hash_set(key_index, key, APR_HASH_KEY_STRING, (void*)index_value);
    --pending_pairs_to_read;
    //printf("jp_import_TLV_key_index_from_file: pending_pairs_to_read: %ld \n", pending_pairs_to_read);
  }

  return key_index;
}
#undef JP_IO_HELPER_BUFFER_SIZE
