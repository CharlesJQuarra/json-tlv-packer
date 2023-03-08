
#include <apr_strings.h>

#include <json_visit.h>

#include "jp_tlv_encoder.h"
#include "jp_tlv_encoder_private.h"



static int jp_write_key_index_pair(void *rec, const void *key, apr_ssize_t klen, const void *value)
{
  jp_buffer_io_t* buffer = rec;
  printf("jp_write_key_index_pair: key %s klen %ld -> val %ld \n", (const char*)key, klen, (size_t)value);

  uint32_t expected_length_to_write = sizeof(uint32_t) + sizeof(uint32_t) + (size_t)klen;

  if (buffer->current_size < expected_length_to_write) {
    if (0 != jp_buffer_io_grow(buffer, expected_length_to_write))
      return 0;
  }

  printf("jp_write_key_index_pair: expected_length_to_write: %d \n", expected_length_to_write);
  printf("jp_write_key_index_pair: index value: %d \n", (uint32_t)value);

  if (jp_buffer_io_bytes_left_to_write(buffer) < expected_length_to_write) {
    jp_buffer_io_flush_writes(buffer);
  }

  expected_length_to_write -= jp_export_uint32_to_buffer(expected_length_to_write, buffer);
  expected_length_to_write -= jp_export_uint32_to_buffer((uint32_t) value, buffer);

  //memcpy(helper->current_buffer + helper->used, key, expected_length_to_write);
  jp_buffer_io_memcpy_to(buffer, key, expected_length_to_write);

  printf("jp_write_key_index_pair: expected_length_to_write to end of segment: %d \n", expected_length_to_write);

  return 1;
}


int jp_export_key_index_to_file(apr_hash_t *key_index,
                                FILE       *output)
{
  jp_buffer_io_t buffer;
  jp_buffer_io_initialize(& buffer, apr_hash_pool_get(key_index), output);

  uint64_t length = apr_hash_count(key_index);

  //memcpy(helper.current_buffer, &length, sizeof(uint64_t));
  jp_buffer_io_memcpy_to(& buffer, & length, sizeof(uint64_t));

  int ret = !apr_hash_do(jp_write_key_index_pair, & buffer, key_index);

  /* write any leftover buffered data */
  jp_buffer_io_flush_writes(& buffer);

  printf("jp_export_key_array_to_file: ret %d \n", ret);
  return ret;
}

apr_hash_t* jp_import_key_index_from_file(apr_pool_t *pool,
                                          FILE       *input)
{
  jp_buffer_io_t buffer;
  jp_buffer_io_initialize(& buffer, pool, input);

  jp_buffer_io_read(& buffer);

  uint64_t length;
  //memcpy(& length, helper.current_buffer, sizeof(uint64_t));
  //helper.used += sizeof(uint64_t);
  jp_buffer_io_memcpy_from(& buffer, & length, sizeof(uint64_t));

  uint64_t pending_pairs_to_read = length;
  printf("jp_import_TLV_key_index_from_file: pending_pairs_to_read: %ld \n", pending_pairs_to_read);

  apr_hash_t* key_index = apr_hash_make(pool);

  while(pending_pairs_to_read > 0) {

    if (jp_buffer_io_bytes_left_to_read(& buffer) < sizeof(uint32_t))
      jp_buffer_io_read(& buffer);

    uint32_t expected_length_to_read;

    expected_length_to_read -= jp_import_uint32_from_buffer(& expected_length_to_read, & buffer);

    printf("jp_import_TLV_key_index_from_file: expected_length_to_read: %d \n", expected_length_to_read);

    if (expected_length_to_read > buffer.current_size) {
      if (0 != jp_buffer_io_grow(& buffer, expected_length_to_read))
        return NULL;
    }

    if (jp_buffer_io_bytes_left_to_read(& buffer) < expected_length_to_read)
      jp_buffer_io_read(& buffer);

    uint32_t index_value;

    expected_length_to_read -= jp_import_uint32_from_buffer(& index_value, & buffer);

    printf("jp_import_TLV_key_index_from_file: index_value: %d expected_length_to_read to end of segment %d  \n", index_value, expected_length_to_read);

    char* key    = apr_pmemdup(pool, jp_buffer_io_use_available_bytes(& buffer, expected_length_to_read), expected_length_to_read + 1);
    char zero    = 0;
    memcpy(key + expected_length_to_read, &zero, 1);

    apr_hash_set(key_index, key, APR_HASH_KEY_STRING, (void*)index_value);
    --pending_pairs_to_read;
  }

  return key_index;
}

#undef JP_IO_HELPER_BUFFER_SIZE
