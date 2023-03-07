
#include <apr_pools.h>

#include "jp_tlv_encoder_private.h"



void
jp_buffer_io_initialize(jp_buffer_io_helper_t* helper, apr_pool_t *pool, FILE* file)
{
  helper->used           = 0;
  helper->read           = 0;
  helper->current_buffer = helper->initial_buffer;
  helper->current_size   = JP_IO_HELPER_BUFFER_SIZE;
  helper->stream         = file;
  helper->pool           = pool;
}

int
jp_buffer_io_grow(jp_buffer_io_helper_t* helper, size_t not_less_than)
{
  size_t new_size = 2 * helper->current_size;

  while (new_size < not_less_than)
    new_size *= 2;

  uint8_t *new_buffer = apr_palloc(helper->pool, new_size);

  if (NULL == new_buffer)
    return -1;

  memcpy(new_buffer, helper->current_buffer, helper->used);
  helper->current_buffer = new_buffer;
  helper->current_size   = new_size;

  return 0;
}

void
jp_buffer_io_flush_writes(jp_buffer_io_helper_t* helper)
{
  fwrite(helper->current_buffer, 1, helper->used, helper->stream);
  helper->used = 0;
}

int
jp_buffer_io_bytes_left_to_read(jp_buffer_io_helper_t* helper)
{
  return helper->read - helper->used;
}

int
jp_buffer_io_read(jp_buffer_io_helper_t* helper)
{
  int leftover_read = jp_buffer_io_bytes_left_to_read(helper);

  if (leftover_read < 0)
    return -1;

  if (leftover_read > 0)
    memmove(helper->current_buffer, helper->current_buffer + helper->used, leftover_read);

  helper->read = fread(helper->current_buffer + leftover_read, 1, helper->current_size, helper->stream);
  helper->eof  = feof(helper->stream);
  helper->used = 0;

  return 0;
}