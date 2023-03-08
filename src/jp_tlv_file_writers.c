

#include <jp_tlv_encoder.h>
#include "jp_tlv_encoder_private.h"

int jp_export_records_to_file_set(jp_TLV_records_t *record_collection,
                                  FILE             *kv_pair_output,
                                  FILE             *key_index_output)
{
  jp_buffer_io_t buffer;

  jp_buffer_io_initialize(& buffer, apr_hash_pool_get(record_collection->key_index), kv_pair_output);


  jp_export_key_index_to_file(record_collection->key_index, key_index_output);
}

int jp_import_records_from_file_set(jp_TLV_records_t *record_collection,
                                    FILE             *kv_pair_input,
                                    FILE             *key_index_input)
{
}