
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

  if (argc % 2 != 1) {
    fprintf(stderr, "Expecting two input files per set to consolidate (a kv-pair TLV file and a key index TLV file, in that order), received %d. Exiting \n", argc - 1);

    rv = -1;
    goto terminate;
  }

  if (argc < 5) {
    fprintf(stderr, "Received a single file set, nothing to consolidate, exiting \n");

    rv = -1;
    goto terminate;
  }

  const char* consolidated_key_index_out = "consolidated_key_index.tlv";
  const char* consolidated_kv_pair_out   = "consolidated_kv_pair.tlv";

  apr_pool_create(&p, NULL);

  jp_TLV_records_t* tlv_records = jp_TLV_record_collection_make(p);


  for(int i = 1; i < argc; i += 2)
  {
    const char* kv_pair_input   = argv[i];
    const char* key_index_input = argv[i+1];

    printf("processing file set: %s - %s \n", kv_pair_input, key_index_input);

    FILE* kv_pair_file   = open_filename(kv_pair_input, "rb", 0);
    FILE* key_index_file = open_filename(key_index_input, "rb", 0);

    jp_import_records_from_file_set(tlv_records, kv_pair_file, key_index_file);

    close_filename(kv_pair_input, kv_pair_file);
    close_filename(key_index_input, key_index_file);
  }

  {
    FILE* consolidated_kv_pair_file   = open_filename(consolidated_kv_pair_out, "wb", 0);
    FILE* consolidated_key_index_file = open_filename(consolidated_key_index_out, "wb", 0);

    jp_export_records_to_file_set(tlv_records, consolidated_kv_pair_file, consolidated_key_index_file);

    close_filename(consolidated_key_index_out, consolidated_key_index_file);
    close_filename(consolidated_kv_pair_out, consolidated_kv_pair_file);

    printf("consolidated file set: %s - %s. Success\n", consolidated_kv_pair_out, consolidated_key_index_out);
    rv = -1;
  }

  terminate:
  apr_terminate();
  return rv;
};
