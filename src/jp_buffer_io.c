
#include <apr_pools.h>

#include "jp_tlv_encoder_private.h"


static void jp_buffer_io_initialize(jp_buffer_io_t* buffer, apr_pool_t *pool, FILE* file, int read_mode)
{
  buffer->used           = 0;
  buffer->read           = 0;
  buffer->current_buffer = buffer->initial_buffer;
  buffer->current_size   = JP_IO_HELPER_BUFFER_SIZE;
  buffer->stream         = file;
  buffer->pool           = pool;
  buffer->read_mode      = read_mode;
}

void
jp_buffer_io_read_initialize(jp_buffer_io_t* buffer, apr_pool_t *pool, FILE* file)
{
  jp_buffer_io_initialize(buffer, pool, file, 1);
}

void
jp_buffer_io_write_initialize(jp_buffer_io_t* buffer, apr_pool_t *pool, FILE* file)
{
  jp_buffer_io_initialize(buffer, pool, file, 0);
}

void
jp_buffer_io_initialize_static(jp_buffer_io_t *buffer, uint8_t* memory, size_t size)
{
  buffer->used           = 0;
  buffer->read           = 0;
  buffer->current_buffer = (NULL == memory) ? buffer->initial_buffer   : memory;
  buffer->current_size   = (NULL == memory) ? JP_IO_HELPER_BUFFER_SIZE : size;
  buffer->stream         = NULL;
  buffer->pool           = NULL;
  buffer->read_mode      = 0;
}

int
jp_buffer_io_grow(jp_buffer_io_t* buffer, size_t not_less_than)
{
  if (NULL == buffer->pool)
    return -1;

  size_t new_size = 2 * buffer->current_size;

  while (new_size < not_less_than)
    new_size *= 2;

  uint8_t *new_buffer = apr_palloc(buffer->pool, new_size);

  if (NULL == new_buffer)
    return -1;

  size_t cpy_to_offset = 0;
  if (buffer->read_mode)
    cpy_to_offset = buffer->read;
  else
    cpy_to_offset = buffer->used;

  memcpy(new_buffer, buffer->current_buffer, cpy_to_offset);
  buffer->current_buffer = new_buffer;
  buffer->current_size   = new_size;

  return 0;
}

uint8_t* jp_buffer_io_use_available_bytes(jp_buffer_io_t *buffer,
                                          size_t          size)
{
  if (buffer->used + size < buffer->current_size) {
    uint8_t* ret = buffer->current_buffer + buffer->used;

    buffer->used += size;

    return ret;
  }

  fprintf(stderr, "jp_buffer_io_use_available_bytes: trying to consume %ld bytes, but only %ld available \n",size, buffer->current_size - buffer->used);
  fprintf(stderr, "jp_buffer_io_use_available_bytes: buffer size: %ld. used: %ld. Returning NULL \n", buffer->current_size, buffer->used);

  return NULL;
}

int jp_buffer_io_bytes_left_to_write(jp_buffer_io_t *buffer)
{
  return buffer->current_size - buffer->used;
}

int
jp_buffer_io_bytes_left_to_read(jp_buffer_io_t* buffer)
{
  if (NULL == buffer->stream)
    return buffer->current_size - buffer->used;

  return buffer->read - buffer->used;
}

void* jp_buffer_io_memcpy_to(      jp_buffer_io_t *buffer,
                             const void           *src,
                                   size_t          size)
{
  if (jp_buffer_io_bytes_left_to_write(buffer) < size)
    return NULL;

  void *ret = memcpy(buffer->current_buffer + buffer->used, src, size);

  buffer->used += size;

  return ret;
}

void* jp_buffer_io_memcpy_from(jp_buffer_io_t *buffer,
                               void           *dest,
                               size_t          size)
{
  if (jp_buffer_io_bytes_left_to_read(buffer) < size)
    return NULL;

  void *ret = memcpy(dest, buffer->current_buffer + buffer->used, size);

  buffer->used += size;

  return ret;
}


void jp_buffer_io_flush_writes(jp_buffer_io_t* buffer)
{
  if (buffer->stream)
    fwrite(buffer->current_buffer, 1, buffer->used, buffer->stream);

  buffer->used = 0;
}

int
jp_buffer_io_read(jp_buffer_io_t* buffer)
{
  int leftover_read = jp_buffer_io_bytes_left_to_read(buffer);

  if (leftover_read < 0)
    return -1;

  if (leftover_read > 0) {
    memmove(buffer->current_buffer, buffer->current_buffer + buffer->used, leftover_read);
  }

  if (buffer->stream) {
    buffer->read = leftover_read + fread(buffer->current_buffer + leftover_read, 1, buffer->current_size, buffer->stream);
    buffer->eof  = feof(buffer->stream);
  }
  else return -1;

  buffer->used = 0;

  return buffer->read == 0;
}
