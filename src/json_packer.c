
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

  const char* inputfile       = (argc > 1) ? argv[1] : NULL;
  const char* keyarrayoutfile = (argc > 2) ? argv[2] : "key_array.bin";
  const char* kvpairoutfile   = (argc > 3) ? argv[3] : "kv_pair.bin";

  apr_pool_create(&p, NULL);

  jp_TLV_records_t* tlv_records = jp_TLV_record_collection_make(p);

  if (NULL == inputfile) {
    fprintf(stderr, "No input JSON file\n");

    rv = -1;
    goto terminate;
  }

  FILE* input = open_filename(inputfile, "r", 1);
  jp_update_records_from_json_file(p, tlv_records, input);

  apr_hash_t* key_index = tlv_records->key_index;
  apr_hash_t* index_key = jp_build_index_2_key_from_key_index(key_index);

  for (apr_hash_index_t *hi = apr_hash_first(NULL, index_key); hi; hi = apr_hash_next(hi)) {
      size_t      k;
      const char *v;

      apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
      printf("ht iteration: key=%ld, val=%s\n", k, v);
  }

  close_filename(inputfile, input);

  if (keyarrayoutfile) {
    FILE* out = open_filename(keyarrayoutfile, "wb", 0);
    jp_export_key_index_to_file(key_index, out);

    close_filename(keyarrayoutfile, out);

    FILE* readkeyarrayfile = open_filename(keyarrayoutfile, "rb", 1);
    apr_hash_t* key_index_from_file = jp_import_key_index_from_file(p, readkeyarrayfile);

    if (NULL == key_index_from_file) {
      printf("errors reading %s\n", keyarrayoutfile);
    } else {
      printf("ready to loop on key_index_from_file \n");
      for (apr_hash_index_t *hi = apr_hash_first(NULL, key_index_from_file); hi; hi = apr_hash_next(hi)) {
        size_t      v;
        const char *k;

        apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
        printf("key_index_from_file: key=%s, val=%ld\n", k, v);
      }
    }
  }

  if (kvpairoutfile) {
    FILE* out = open_filename(kvpairoutfile, "wb", 0);

  }

  terminate:
  apr_terminate();
  return rv;
};
