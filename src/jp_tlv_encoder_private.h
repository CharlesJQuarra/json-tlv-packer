
#include <jp_tlv_encoder.h>

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