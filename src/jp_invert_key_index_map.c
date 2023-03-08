
#include <json_visit.h>

#include "jp_tlv_encoder.h"

apr_array_header_t* jp_build_key_array_from_key_index(apr_hash_t* key_index)
{
  unsigned int nb_elems = apr_hash_count(key_index);
  apr_array_header_t* key_array = apr_array_make(apr_hash_pool_get(key_index), nb_elems, sizeof(const char**));

  for (apr_hash_index_t *hi = apr_hash_first(NULL, key_index); hi; hi = apr_hash_next(hi)) {
      size_t      v;
      const char *k;

      apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
      const char** key_slot = & ((const char**) key_array->elts)[v - 1];
      *key_slot = k;
  }

  return key_array;
}