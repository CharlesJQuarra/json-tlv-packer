
#ifndef JP_TLV_ENCODER
#define JP_TLV_ENCODER

#include <apr_hash.h>
#include <apr_pools.h>
#include <apr_tables.h>

#include <json_object.h>

#include <stdio.h>

typedef struct jp_TLV_records
{

  apr_hash_t         *key_index;
  apr_array_header_t *record_list;

} jp_TLV_records_t;

typedef struct jp_TLV_kv_pair
{

  uint32_t key_index;
  uint32_t value_length;
  void*    value_buffer;
  uint32_t value_type;

} jp_TLV_kv_pair_t;

typedef struct jp_TLV_record
{

  apr_array_header_t *kv_pairs_array;

} jp_TLV_record_t;

/**
 * Gets the TLV layout size of a key-value pair
 *
 * @param kv_pair A key-value pair
 *
 * @returns The layout size in bytes
 */
size_t jp_get_TLV_kv_pair_layout_size(jp_TLV_kv_pair_t* kv_pair);

/**
 * Gets the TLV layout size of a TLV record
 *
 * @param record A TLV record
 *
 * @returns The layout size in bytes
 */
size_t jp_get_TLV_record_layout_size(jp_TLV_record_t* record);

/**
 * Creates an instance of a TLV record collection
 *
 * @param pool A memory pool
 *
 * @returns A pointer to the new instance
 */
jp_TLV_records_t* jp_TLV_record_collection_make(apr_pool_t *pool);

/**
 * Creates an instance of a TLV record
 *
 * @param pool A memory pool
 *
 * @returns A pointer to the new instance
 */
jp_TLV_record_t* jp_TLV_record_make(apr_pool_t *pool);

/**
 * Adds an existing record to a collection
 *
 * @param record_collection The collection
 * @param record            The record to be added
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_add_record_to_TLV_collection(jp_TLV_records_t *record_collection,
                                    jp_TLV_record_t  *record);

/**
 * Finds or adds the position of a key in the records
 *
 * @param records The TLV encoded records
 * @param key     The key to retrieve
 *
 * @returns The index of the key in the records
 */
size_t jp_find_or_add_key(jp_TLV_records_t *records,
                          const char*       key);

/**
 * Adds a key-value pair to a TLV record with a boolean value
 *
 * @param record    The record that owns the key-value pair
 * @param key_index The key index
 * @param value     A boolean value
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_add_boolean_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value);

/**
 * Adds a key-value pair to a TLV record with a string value
 *
 * @param record    The record that owns the key-value pair
 * @param key_index The key index
 * @param value     A string value
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_add_string_kv_pair_to_record(jp_TLV_record_t *record,
                                    size_t           key_index,
                                    const char*      value);

/**
 * Adds a key-value pair to a TLV record with an integer value
 *
 * @param record    The record that owns the key-value pair
 * @param key_index The key index
 * @param value     A integer value
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_add_integer_kv_pair_to_record(jp_TLV_record_t *record,
                                     size_t           key_index,
                                     int              value);

/**
 * Adds a key-value pair to a TLV record with an floating-point value
 *
 * @param record    The record that owns the key-value pair
 * @param key_index The key index
 * @param value     A floating-point value
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_add_double_kv_pair_to_record(jp_TLV_record_t *record,
                                    size_t           key_index,
                                    double           value);

/**
 * Incrementally updates the TLV records with a new json_object
 *
 * @param pool    A memory pool
 * @param records The TLV encoded records
 * @param jso     A new json record object
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_update_records_from_json(apr_pool_t       *pool,
                                jp_TLV_records_t *records,
                                json_object      *jso);


/**
 * Incrementally updates the TLV records from an input JSON file
 *
 * @param pool    A memory pool
 * @param records The TLV encoded records
 * @param input   An input file with one json record per line
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_update_records_from_file(apr_pool_t       *pool,
                                jp_TLV_records_t *tlv_records,
                                FILE             *input);

/**
 *  Exports a TLV record key-value pair to a buffer
 *
 *  @param kv_pair     The key-value pair to export
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, -1 if the buffer_size was too small
 */
int32_t jp_export_kv_pair_to_buffer(jp_TLV_kv_pair_t *kv_pair,
                                    char             *buffer,
                                    size_t            buffer_size);

/**
 *  Exports a TLV record to a buffer
 *
 *  @param record      The TLV record to export
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, -1 if the buffer_size was too small
 */
int32_t jp_export_record_to_buffer(jp_TLV_record_t *record,
                                   char            *buffer,
                                   size_t           buffer_size);

/**
 *  Exports a TLV record to a file
 *
 *  @param record The TLV record to export
 *  @param output The output file
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_export_record_to_file(jp_TLV_record_t *record,
                             FILE            *output);

/**
 *  Exports the TLV key array to a file
 *
 *  @param key_index The TLV key index to export
 *  @param output    The output file
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_export_key_array_to_file(apr_hash_t *key_index,
                                FILE       *output);

/**
 *  Imports A TLV key index from a binary file
 *
 * @param pool   A memory pool
 *  @param input The input TLV key index file
 *
 * @returns A TLV key index
 */
apr_hash_t* jp_import_TLV_key_index_from_file(apr_pool_t *pool,
                                              FILE       *input);

/**
 *  Imports A TLV record collection from a binary file
 *
 *  @param key_index The TLV key index to export
 *  @param input     The input TLV record collection file
 *
 * @returns A TLV record collection
 */
jp_TLV_records_t* jp_import_TLV_record_collection_from_file(apr_hash_t *key_index,
                                                            FILE       *input);

/**
 *  Builds The inverse index key map from a key index map
 *
 *  @param input The TLV key index
 *
 * @returns A TLV index map
 */
apr_hash_t* jp_build_index_2_key_from_key_index(apr_hash_t* key_index);

#endif /* JP_TLV_ENCODER */
