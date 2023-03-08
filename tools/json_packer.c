
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
  const char* kvpairoutfile   = (argc > 2) ? argv[2] : "kv_pair.tlv";
  const char* keyarrayoutfile = (argc > 3) ? argv[3] : "key_index.tlv";

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
  close_filename(inputfile, input);

  if (kvpairoutfile) {
    FILE* kvpairout = open_filename(kvpairoutfile, "wb", 0);
    FILE* kindexout = open_filename(keyarrayoutfile, "wb", 0);

    jp_export_records_to_file_set(tlv_records, kvpairout, kindexout);

    close_filename(keyarrayoutfile, kindexout);
    close_filename(kvpairoutfile, kvpairout);

    rv = 0;
  }

  terminate:
  apr_terminate();
  return rv;
};
