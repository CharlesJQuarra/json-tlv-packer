
#include <apr_strings.h>

#include <json_visit.h>

#include "jp_tlv_encoder.h"


#define JP_IO_HELPER_BUFFER_SIZE 4096
typedef struct jp_buffer_io_helper
{
  size_t used;
  char   buffer[JP_IO_HELPER_BUFFER_SIZE];
  int    eof;
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

int jp_export_key_index_to_file(apr_hash_t *key_index,
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

apr_hash_t* jp_import_key_index_from_file(apr_pool_t *pool,
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

  uint64_t pending_pairs_to_read = length;
  printf("jp_import_TLV_key_index_from_file: pending_pairs_to_read: %ld \n", pending_pairs_to_read);

  read       = fread(helper.buffer, 1, JP_IO_HELPER_BUFFER_SIZE, input);
  helper.eof = feof(input);

  apr_hash_t* key_index = apr_hash_make(pool);

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
