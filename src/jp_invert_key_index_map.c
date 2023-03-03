
#include <json_visit.h>

#include "jp_tlv_encoder.h"


static int jp_invert_key_index_iterate(void *rec, const void *key, apr_ssize_t klen, const void *value)
{
  apr_hash_t* index_key = rec;
  printf("jp_invert_key_index_iterate: key %s -> val %ld \n", (const char*)key, (size_t)value);
  apr_hash_set(index_key, value, sizeof(size_t), key);
  return 1;
}

static unsigned int jp_index_key_hash(const char *key, apr_ssize_t *klen)
{
  return (unsigned int)key;
}

apr_hash_t* jp_build_index_2_key_from_key_index(apr_hash_t* key_index)
{
  apr_hash_t* index_key = apr_hash_make_custom(apr_hash_pool_get(key_index), jp_index_key_hash);

  apr_hash_do(jp_invert_key_index_iterate, index_key, key_index);

  return index_key;
}