

#include <jp_tlv_encoder.h>
#include "jp_tlv_encoder_private.h"

int jp_export_records_to_file_set(jp_TLV_records_t *record_collection,
                                  FILE             *kv_pair_output,
                                  FILE             *key_index_output)
{
  {
    jp_buffer_io_t buffer;
    jp_buffer_io_write_initialize(& buffer, apr_hash_pool_get(record_collection->key_index), kv_pair_output);

    apr_array_header_t* record_array = record_collection->record_list;
    uint32_t            nb_records   = record_array->nelts;

    if (0 == jp_export_uint32_to_buffer(nb_records, & buffer))
        return -1;

    for (int i = 0; i < nb_records; i++) {
      jp_TLV_record_t* record =  ((jp_TLV_record_t**) record_array->elts)[i];

      if (0 == jp_export_record_to_buffer(record, & buffer))
        return -1;
    }

    jp_buffer_io_flush_writes(& buffer);
  }

  return jp_export_key_index_to_file(record_collection->key_index, key_index_output);
}

int jp_import_records_from_file_set(jp_TLV_records_t *record_collection,
                                    FILE             *kv_pair_input,
                                    FILE             *key_index_input)
{
  apr_pool_t* pool = apr_hash_pool_get(record_collection->key_index);

  apr_hash_t*         file_key_index = jp_import_key_index_from_file(pool, key_index_input);
  apr_array_header_t* file_key_array = jp_build_key_array_from_key_index(file_key_index);

  {
    jp_buffer_io_t buffer;
    jp_buffer_io_read_initialize(& buffer, pool, kv_pair_input);

    uint32_t nb_records;

    if (0 == jp_import_uint32_from_buffer(& nb_records, & buffer))
      return -1;

    for (int i = 0; i < nb_records; i++) {
      jp_TLV_record_t* record;

      if (0 == jp_import_record_from_buffer(pool, & record, & buffer))
        return -1;

      apr_array_header_t* kv_array = record->kv_pairs_array;
      uint32_t            nb_pairs = kv_array->nelts;

      for (int j = 0; j < nb_pairs; j++) {
        jp_TLV_kv_pair_t* kv_pair = & ((jp_TLV_kv_pair_t*) kv_array->elts)[j];

        const char* key_on_source = ((const char**) file_key_array->elts)[kv_pair->key_index - 1];
        size_t index_on_target    = jp_find_or_add_key(record_collection->key_index, key_on_source);
        kv_pair->key_index        = index_on_target;
      }

      jp_add_record_to_TLV_collection(record_collection, record);
    }
  }
}