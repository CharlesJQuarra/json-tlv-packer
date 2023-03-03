
#ifndef JP_TLV_ENCODER
#define JP_TLV_ENCODER

#include <apr_hash.h>
#include <apr_pools.h>
#include <apr_tables.h>

#include <json_object.h>

typedef struct jp_TLV_records
{
  apr_hash_t          *key_index;
  apr_array_header_t **record_list;

} jp_TLV_records_t;

typedef struct jp_TLV_kv_pair
{

  size_t   key_index;
  size_t   value_length;
  void*    value_buffer;
  uint32_t value_type;

} jp_TLV_kv_pair_t;

typedef struct jp_TLV_record
{

  jp_TLV_records     *parent_collection;
  apr_array_header_t *kv_pairs_array;

} jp_TLV_record_t;

/**
 * Finds or adds the position of a key in the records
 *
 * @param records The TLV encoded records
 * @param key     The key to retrieve
 *
 * @returns The index of the key in the records
 */
size_t find_or_add_key(jp_TLV_records_t *records,
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
int add_boolean_kv_pair_to_record(jp_TLV_record_t *record,
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
int add_string_kv_pair_to_record(jp_TLV_record_t *record,
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
int add_integer_kv_pair_to_record(jp_TLV_record_t *record,
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
int add_double_kv_pair_to_record(jp_TLV_record_t *record,
                                 size_t           key_index,
                                 double           value);

/**
 * Creates an instance of a TLV record
 *
 * @param pool A memory pool
 *
 * @returns A pointer to the new instance
 */
jp_TLV_record_t* jp_TLV_record_make(apr_pool_t *pool);

/**
 * Incrementally updates the TLV records with a new json_object
 *
 * @param pool    A memory pool
 * @param records The TLV encoded records
 * @param jso     A new json record object
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_update_records_from_json(apr_pool_t        *pool,
                                jp_TLV_records_t  *records,
                                const json_object *jso);


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
 *  @param key_array The TLV key array to export
 *  @param output    The output file
 *
 * @returns zero if succeeded, non-zero if an error condition occurred
 */
int jp_export_key_array_to_file(apr_array_header_t *key_array,
                                FILE               *output);

#endif /* JP_TLV_ENCODER */
