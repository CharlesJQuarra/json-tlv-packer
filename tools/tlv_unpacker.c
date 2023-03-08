
#include <apr.h>
#include <apr_hash.h>

#include <stdio.h>
#include <stdlib.h>

#include "jp_tlv_encoder.h"


/* file utils */

FILE *open_filename(const char *filename, const char *opt, int is_input)
{
	FILE *input;
	if (strcmp(filename, "-") == 0)
		input = (is_input) ? stdin : stdout;
	else {
		input = fopen(filename, opt);
		if (!input) {
			fprintf(stderr, "error: cannot open %s: %s", filename, strerror(errno));
			return NULL;
		}
	}
	return input;
}


void close_filename(const char *filename, FILE *file)
{
	if (file != NULL && strcmp(filename, "-") != 0)
		fclose(file);
}


int main(int                argc,
         const char* const *argv)
{
  apr_status_t rv;
  apr_pool_t  *p = NULL;

  apr_app_initialize(&argc, &argv, NULL);
  atexit(apr_terminate);

  const char* kvpairinfile   = (argc > 1) ? argv[1] : "kv_pair.tlv";
  const char* keyarrayinfile = (argc > 2) ? argv[2] : "key_index.tlv";

  apr_pool_create(&p, NULL);

  jp_TLV_records_t* tlv_records = jp_TLV_record_collection_make(p);

  {
    FILE* kvpairin = open_filename(kvpairinfile, "rb", 0);
    FILE* kindexin = open_filename(keyarrayinfile, "rb", 0);

    jp_import_records_from_file_set(tlv_records, kvpairin, kindexin);

    close_filename(keyarrayinfile, kindexin);
    close_filename(kvpairinfile, kvpairin);
  }

  apr_array_header_t* key_array    = jp_build_key_array_from_key_index(tlv_records->key_index);
  apr_array_header_t* record_array = tlv_records->record_list;

  for (int i = 0; i < record_array->nelts; i++) {
    jp_TLV_record_t* record = ((jp_TLV_record_t**) record_array->elts)[i];

    apr_array_header_t* kv_pairs_array = record->kv_pairs_array;

    for (int j = 0; j < kv_pairs_array->nelts; j++) {
      jp_TLV_kv_pair_t* elem = & ((jp_TLV_kv_pair_t*)kv_pairs_array->elts)[j];

      const char* key = ((const char**) key_array->elts)[elem->key_index - 1];
      printf(" record[ %d ]: key=%s, ", i, key);

      switch(elem->value_type) {
        case JP_TYPE_BOOLEAN:
        {
          int value;
          jp_read_boolean_from_kv_pair(elem, & value);

          printf(" value=%s \n", value ? "true" : "false");
        }
        break;

        case JP_TYPE_INTEGER:
        {
          uint32_t value;
          jp_read_integer_from_kv_pair(elem, & value);

          printf(" value=%d \n", value);
        }
        break;

        case JP_TYPE_DOUBLE:
        {
          double value;
          jp_read_double_from_kv_pair(elem, & value);

          printf(" value=%f \n", value);
        }
        break;

        case JP_TYPE_STRING:
        {
          char *value;
          jp_read_string_from_kv_pair(elem, & value);

          printf(" value=%s \n", value);
        }
        break;

        default:
        break;
      }
    }
  }

  terminate:
  apr_terminate();
  return 0;
};
