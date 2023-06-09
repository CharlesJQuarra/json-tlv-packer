
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

typedef struct jp_TLV_string
{
  uint32_t value_length;
  char*    value_buffer;
} jp_TLV_string_t;

typedef union jp_TLV_union {

  int32_t         integer_value;
  double          double_value;
  jp_TLV_string_t string_value;

} jp_TLV_union_t;

#define JP_TYPE_BOOLEAN   0
#define JP_TYPE_INTEGER   1
#define JP_TYPE_DOUBLE    2
#define JP_TYPE_STRING    3

typedef struct jp_TLV_kv_pair
{

  uint32_t       key_index;
  uint32_t       value_type;
  jp_TLV_union_t union_v;

} jp_TLV_kv_pair_t;

typedef struct jp_TLV_record
{

  apr_array_header_t *kv_pairs_array;

} jp_TLV_record_t;


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
 * @param key_index The hash that maps string keys to indices
 * @param key       The key to retrieve
 *
 * @returns The index of the key in the records
 */
size_t jp_find_or_add_key(apr_hash_t       *key_index,
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
 * Reads a boolean value from a key-value pair if it is the correct type
 *
 * @param kv_pair   The key-value pair
 * @param value     A boolean value
 *
 * @returns zero if succeeded, non-zero if it is not the correct type
 */
int jp_read_boolean_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                       int              *value);

/**
 * Reads a string value from a key-value pair if it is the correct type
 *
 * @param kv_pair   The key-value pair
 * @param value     A string value
 *
 * @returns zero if succeeded, non-zero if it is not the correct type
 */
int jp_read_string_from_kv_pair(const jp_TLV_kv_pair_t  *kv_pair,
                                      char             **value);

/**
 * Reads a integer value from a key-value pair if it is the correct type
 *
 * @param kv_pair   The key-value pair
 * @param value     A integer value
 *
 * @returns zero if succeeded, non-zero if it is not the correct type
 */
int jp_read_integer_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                       int32_t          *value);

/**
 * Reads a double value from a key-value pair if it is the correct type
 *
 * @param kv_pair   The key-value pair
 * @param value     A double value
 *
 * @returns zero if succeeded, non-zero if it is not the correct type
 */
int jp_read_double_from_kv_pair(const jp_TLV_kv_pair_t *kv_pair,
                                      double           *value);

/**
 * Incrementally updates the TLV records with a new json_object
 *
 * @param pool              A memory pool
 * @param record_collection The TLV record collection
 * @param jso               A new json record object
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_update_records_from_json(apr_pool_t       *pool,
                                jp_TLV_records_t *record_collection,
                                json_object      *jso);

/**
 *  Exports a TLV record key-value pair to a static buffer
 *
 *  @param kv_pair     The key-value pair to export
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_export_kv_pair_to_static_buffer(jp_TLV_kv_pair_t *kv_pair,
                                            uint8_t          *buffer,
                                            size_t            buffer_size);

/**
 *  Imports a TLV record key-value pair from a static buffer
 *
 *  @param pool        A memory pool
 *  @param kv_pair     The key-value pair to import
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes read from the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_import_kv_pair_from_static_buffer(apr_pool_t       *pool,
                                              jp_TLV_kv_pair_t *kv_pair,
                                              uint8_t          *buffer,
                                              size_t            buffer_size);

/**
 *  Exports a TLV record to a static buffer
 *
 *  @param record      The TLV record to export
 *  @param buffer      The output buffer
 *  @param buffer_size The maximum allowed written bytes to the buffer
 *
 * @returns bytes written to the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_export_record_to_static_buffer(const jp_TLV_record_t *record,
                                                 uint8_t         *buffer,
                                                 size_t           buffer_size);

/**
 *  Imports a TLV record from a static buffer
 *
 *  @param pool        A memory pool
 *  @param record      The TLV record to write
 *  @param buffer      The input buffer
 *  @param buffer_size The maximum allowed bytes to read from the buffer
 *
 * @returns bytes read from the buffer, 0 if the buffer_size was too small
 */
uint32_t jp_import_record_from_static_buffer(apr_pool_t       *pool,
                                             jp_TLV_record_t **record,
                                             uint8_t          *buffer,
                                             size_t            buffer_size);


/**
 *  Exports the TLV key index to a file
 *
 *  @param key_index The TLV key index to export
 *  @param output    The output file
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_export_key_index_to_file(apr_hash_t *key_index,
                                FILE       *output);

/**
 *  Imports A TLV key index from a binary file
 *
 * @param pool   A memory pool
 *  @param input The input TLV key index file
 *
 * @returns A TLV key index
 */
apr_hash_t* jp_import_key_index_from_file(apr_pool_t *pool,
                                          FILE       *input);


/**
 *  Builds The inverse key array from a key index map
 *
 *  @param input The key index
 *
 * @returns A key array
 */
apr_array_header_t* jp_build_key_array_from_key_index(apr_hash_t* key_index);

/**
 * Incrementally updates the TLV records from an input JSON file
 *
 * @param pool              A memory pool
 * @param record_collection The TLV record collection
 * @param input             An input file with one json record per line
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 *
 * @remarks More than one JSON file can be used to update the same record collection
 */
int jp_update_records_from_json_file(apr_pool_t       *pool,
                                     jp_TLV_records_t *record_collection,
                                     FILE             *input);


/**
 *  Exports a TLV record to a file set
 *
 *  @param record_collection The TLV record to export
 *  @param kv_pair_output    The output TLV key-value records file
 *  @param key_index_output  The output key index file
 *
 *  @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_export_records_to_file_set(jp_TLV_records_t *record_collection,
                                  FILE             *kv_pair_output,
                                  FILE             *key_index_output);


/**
 *  Imports a TLV record from a file set
 *
 *  @param record_collection The TLV record collection to append to
 *  @param kv_pair_input     The input TLV key-value records file
 *  @param key_index_input   The input key index file
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 *
 * @remarks More than one set of kv-pairs/key index files can be used to update the same record collection
 */
int jp_import_records_from_file_set(jp_TLV_records_t *record_collection,
                                    FILE             *kv_pair_input,
                                    FILE             *key_index_input);


#endif /* JP_TLV_ENCODER */
