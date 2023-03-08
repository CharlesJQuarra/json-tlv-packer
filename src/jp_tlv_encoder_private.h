
#include <jp_tlv_encoder.h>


#define JP_IO_HELPER_BUFFER_SIZE 4096
typedef struct jp_buffer_io
{
  size_t      used;
  size_t      read;
  size_t      current_size;
  uint8_t     initial_buffer[JP_IO_HELPER_BUFFER_SIZE];
  uint8_t    *current_buffer;
  int         eof;
  FILE       *stream;
  apr_pool_t *pool;
  int         read_mode;

} jp_buffer_io_t;

/**
 * Initializes an instance of the I/O buffer for reading
 *
 * @param buffer  A pointer to the buffer
 * @param pool    A memory pool
 * @param file    An input file stream
 */
void jp_buffer_io_read_initialize(jp_buffer_io_t *buffer,
                                  apr_pool_t     *pool,
                                  FILE           *file);

/**
 * Initializes an instance of the I/O buffer for writing
 *
 * @param buffer  A pointer to the buffer
 * @param pool    A memory pool
 * @param file    An output file stream
 */
void jp_buffer_io_write_initialize(jp_buffer_io_t *buffer,
                                   apr_pool_t     *pool,
                                   FILE           *file);


/**
 * Initializes an instance I/O buffer pointing to static memory
 *
 * @param buffer  A pointer to the buffer
 * @param memory  The static memory to use as buffer
 * @param size    The maximum size for the fixed memory
 */
void jp_buffer_io_initialize_static(jp_buffer_io_t *buffer,
                                    uint8_t        *memory,
                                    size_t          size);

/**
 * Grows the buffer allocated size
 *
 * @param buffer         A pointer to the buffer
 * @param not_less_than  Minimum required bytes on the buffer
 *
 * @returns zero if the allocation succeeds, non-zero otherwise
 */
int jp_buffer_io_grow(jp_buffer_io_t *buffer,
                      size_t          not_less_than);

/**
 * Used a block of available bytes from the buffer
 *
 * @param buffer  A pointer to the buffer
 * @param size    The number of bytes to use
 *
 * @returns the next available buffer byte, NULL if the available bytes left in the buffer are less than size
 */
uint8_t* jp_buffer_io_use_available_bytes(jp_buffer_io_t *buffer,
                                          size_t          size);

/**
 * A bounds checked memcpy that copies onto next available buffer byte
 *
 * @param buffer  A pointer to the buffer
 * @param src     Pointer to the memory source
 * @param size    bytes to copy
 *
 * @returns A pointer to the first copied byte in destination if the buffer is large enough for the copy, NULL otherwise
 */
void* jp_buffer_io_memcpy_to(      jp_buffer_io_t *buffer,
                             const void           *src,
                                   size_t          size);

/**
 * A bounds checked memcpy that copies from next available buffer byte
 *
 * @param buffer  A pointer to the buffer
 * @param dest    Pointer to the memory destination
 * @param size    bytes to copy
 *
 * @returns A pointer to the first copied byte in destination if the buffer has enough bytes left to read, NULL otherwise
 */
void* jp_buffer_io_memcpy_from(jp_buffer_io_t *buffer,
                               void           *dest,
                               size_t          size);

/**
 * Gets remaining bytes to write in the buffer
 *
 * @param buffer  A pointer to the buffer
 *
 * @returns unused bytes left in the buffer
 */
int jp_buffer_io_bytes_left_to_write(jp_buffer_io_t *buffer);

/**
 * Flushes any pending writes to the I/O stream
 *
 * @param buffer  A pointer to the buffer
 */
void jp_buffer_io_flush_writes(jp_buffer_io_t *buffer);

/**
 * Gets remaining unread bytes in the buffer
 *
 * @param buffer  A pointer to the buffer
 *
 * @returns unread bytes in the buffer
 */
int jp_buffer_io_bytes_left_to_read(jp_buffer_io_t *buffer);

/**
 * Reads from the stream into the buffer
 *
 * @param buffer  A pointer to the buffer
 *
 * @returns zero if the read succeeds, non-zero otherwise
 */
int jp_buffer_io_read(jp_buffer_io_t *buffer);

/**
 *  Exports a uint32_t to a buffer
 *
 *  @param value   The uint32_t to export
 *  @param buffer  A pointer to the I/O buffer
 *
 * @returns bytes written to the buffer
 */
uint32_t jp_export_uint32_to_buffer(uint32_t        value,
                                    jp_buffer_io_t *buffer);

/**
 *  Imports a uint32_t from a buffer
 *
 *  @param value   The uint32_t to write
 *  @param buffer  A pointer to the I/O buffer
 *
 * @returns bytes read from the buffer
 */
uint32_t jp_import_uint32_from_buffer(uint32_t       *value,
                                      jp_buffer_io_t *buffer);

/**
 *  Exports a TLV value union to a buffer
 *
 *  @param union_value  The value union to export
 *  @param value_type   The type of union value
 *  @param buffer       A pointer to the I/O buffer
 *
 * @returns bytes written to the buffer
 */
uint32_t jp_export_value_union_to_buffer(const jp_TLV_union_t *union_value,
                                               uint32_t        value_type,
                                               jp_buffer_io_t *buffer);

/**
 *  Imports a TLV value union from a buffer
 *
 *  @param pool         A memory pool
 *  @param union_value  The value union to write
 *  @param value_type   The type of union value
 *  @param buffer       A pointer to the I/O buffer
 *
 * @returns bytes read from the buffer
 */
uint32_t jp_import_value_union_from_buffer(apr_pool_t     *pool,
                                           jp_TLV_union_t *union_value,
                                           uint32_t       *value_type,
                                           jp_buffer_io_t *buffer);


/**
 *  Exports a TLV record key-value pair to a buffer
 *
 *  @param kv_pair  The key-value pair to export
 *  @param buffer   A pointer to the I/O buffer
 *
 * @returns bytes written to the buffer
 */
uint32_t jp_export_kv_pair_to_buffer(jp_TLV_kv_pair_t *kv_pair,
                                     jp_buffer_io_t   *buffer);

/**
 *  Imports a TLV record key-value pair from a buffer
 *
 *  @param pool     A memory pool
 *  @param kv_pair  The key-value pair to import
 *  @param buffer   A pointer to the I/O buffer
 *
 * @returns bytes read from the buffer
 */
uint32_t jp_import_kv_pair_from_buffer(apr_pool_t       *pool,
                                       jp_TLV_kv_pair_t *kv_pair,
                                       jp_buffer_io_t   *buffer);


/**
 *  Exports a TLV record to a buffer
 *
 *  @param record  The TLV record to export
 *  @param buffer  A pointer to the I/O buffer
 *
 * @returns bytes written to the buffer
 */
uint32_t jp_export_record_to_buffer(const jp_TLV_record_t *record,
                                          jp_buffer_io_t  *buffer);

/**
 *  Imports a TLV record from a buffer
 *
 *  @param pool    A memory pool
 *  @param record  The TLV record to write
 *  @param buffer  A pointer to the I/O buffer
 *
 * @returns bytes read from the buffer
 */
uint32_t jp_import_record_from_buffer(apr_pool_t       *pool,
                                      jp_TLV_record_t **record,
                                      jp_buffer_io_t  *buffer);
