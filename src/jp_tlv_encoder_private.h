
#include <jp_tlv_encoder.h>


#define JP_IO_HELPER_BUFFER_SIZE 4096
typedef struct jp_buffer_io_helper
{
  size_t      used;
  size_t      read;
  size_t      current_size;
  uint8_t     initial_buffer[JP_IO_HELPER_BUFFER_SIZE];
  uint8_t    *current_buffer;
  int         eof;
  FILE       *stream;
  apr_pool_t *pool;

} jp_buffer_io_helper_t;

/**
 * Initializes an instance of the buffer I/O helper
 *
 * @param helper A pointer to the helper
 * @param pool   A memory pool
 * @param file   An I/O file stream
 */
void jp_buffer_io_initialize(jp_buffer_io_helper_t *helper,
                             apr_pool_t            *pool,
                             FILE                  *file);

/**
 * Grows the buffer allocated size
 *
 * @param helper        A pointer to the helper
 * @param not_less_than Minimum required bytes on the buffer
 *
 * @returns zero if the allocation succeeds, non-zero otherwise
 */
int jp_buffer_io_grow(jp_buffer_io_helper_t *helper,
                      size_t                 not_less_than);

/**
 * Flushes any pending writes to the I/O stream
 *
 * @param helper A pointer to the helper
 */
void jp_buffer_io_flush_writes(jp_buffer_io_helper_t *helper);

/**
 * Gets remaining unread bytes in the buffer
 *
 * @param helper        A pointer to the helper
 *
 * @returns unread bytes in the buffer
 */
int jp_buffer_io_bytes_left_to_read(jp_buffer_io_helper_t *helper);

/**
 * Reads from the stream into the buffer
 *
 * @param helper        A pointer to the helper
 *
 * @returns zero if the
 */
int jp_buffer_io_read(jp_buffer_io_helper_t *helper);

/**
 *  Exports a uint32_t to a buffer
 *
 *  @param value       The uint32_t to export
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_export_uint32_to_buffer(uint32_t value,
                                    uint8_t *buffer,
                                    size_t   buffer_size);

/**
 *  Imports a uint32_t from a buffer
 *
 *  @param value       The uint32_t to write
 *  @param buffer      The input buffer
 *  @param buffer_size The maximum allowed bytes to read from the buffer
 *
 * @returns bytes read from the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_import_uint32_from_buffer(      uint32_t *value,
                                      const uint8_t  *buffer,
                                            size_t    buffer_size);

/**
 *  Exports a TLV value union to a buffer
 *
 *  @param union_value The value union to export
 *  @param value_type  The type of union value
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_export_value_union_to_buffer(const jp_TLV_union_t *union_value,
                                               uint32_t        value_type,
                                               uint8_t        *buffer,
                                               size_t          buffer_size);

/**
 *  Imports a TLV value union from a buffer
 *
 *  @param pool        A memory pool
 *  @param union_value The value union to write
 *  @param value_type  The type of union value
 *  @param buffer      The input buffer
 *  @param buffer_size The maximum allowed bytes to read from the buffer
 *
 * @returns bytes read from the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_import_value_union_from_buffer(      apr_pool_t     *pool,
                                                 jp_TLV_union_t *union_value,
                                                 uint32_t       *value_type,
                                           const uint8_t        *buffer,
                                                 size_t          buffer_size);