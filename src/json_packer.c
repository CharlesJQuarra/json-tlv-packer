
#include <apr.h>
#include <apr_hash.h>

#include <json.h>
#include <json_visit.h>

#include <stdio.h>

#include "jp_tlv_encoder.h"


json_object*
jp_process_segment(struct json_tokener *tok,
                  const char           *segment,
                  size_t                width)
{
  return json_tokener_parse_ex(tok, segment, width);
}


int jp_process_file(apr_pool_t       *pool,
                    jp_TLV_records_t *tlv_records,
                    FILE             *input)
{
  #define JP_FREAD_BUFFER_SIZE 4
	char                    buffer[JP_FREAD_BUFFER_SIZE];
  char                   *next_line        = NULL;
  int32_t                 line_size        = 0;
  struct json_tokener    *tokener          = json_tokener_new();
	int                     ret              = 0;

	while (1) {
    uint32_t last_parsed_offset = 0, next_segment_size_to_parse = 0;
		int32_t read = fread(buffer, 1, JP_FREAD_BUFFER_SIZE, input);
    int     eof  = feof(input);

		if (read <= 0)
			break;

		for (int i = 0; i < read; i++) {
      int is_last_read          = (i == read - 1);
      int is_terminal_character = (buffer[i] == '\n' || buffer[i] == '\0' || (is_last_read && eof));

      enum json_tokener_error jerr;

			if (is_terminal_character || is_last_read) {
        if (next_line == NULL) {
          next_line = malloc( next_segment_size_to_parse + 1 );
        } else {
          next_line = realloc(next_line, line_size + next_segment_size_to_parse + 1);
        }

        memcpy(next_line + line_size, buffer + last_parsed_offset, next_segment_size_to_parse + 1);
        last_parsed_offset        += next_segment_size_to_parse + 1;
        line_size                 += next_segment_size_to_parse + 1;
        next_segment_size_to_parse = 0;

        if (is_terminal_character) {
          json_object* next_line_object = jp_process_segment(tokener, next_line, line_size);
          free(next_line);
          next_line = NULL;
          line_size = 0;

          if (next_line_object) {
            jp_update_records_from_json(pool, tlv_records, next_line_object);
            json_tokener_reset(tokener);
            json_object_put(next_line_object);
            next_line_object = NULL;
          }
          else if ((jerr = json_tokener_get_error(tokener)) != json_tokener_continue) {
            fprintf(stderr, "JSON Tokener Error: %s\n", json_tokener_error_desc(jerr));
            json_tokener_free(tokener);
            return 1;
          }
        }

      } else
        next_segment_size_to_parse++;
		}
	}

  json_tokener_free(tokener);
	return ret;

  #undef JP_FREAD_BUFFER_SIZE
}

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
	if (strcmp(filename, "-") != 0)
		fclose(file);
}


int main(int                argc,
         const char* const *argv)
{
  apr_status_t rv;
  apr_pool_t  *p = NULL;

  apr_app_initialize(&argc, &argv, NULL);
  atexit(apr_terminate);

  apr_pool_create(&p, NULL);

  jp_TLV_records_t* tlv_records = jp_TLV_record_collection_make(p);

  FILE* input = open_filename(argv[1], "r", 1);
  jp_process_file(p, tlv_records, input);

  apr_hash_t* key_index = tlv_records->key_index;
  apr_hash_t* index_key = jp_build_index_2_key_from_key_index(key_index);

  for (apr_hash_index_t *hi = apr_hash_first(NULL, index_key); hi; hi = apr_hash_next(hi)) {
      size_t      k;
      const char *v;

      apr_hash_this(hi, (const void**)&k, NULL, (void**)&v);
      printf("ht iteration: key=%ld, val=%s\n", k, v);
  }

  close_filename(argv[1], input);

  apr_terminate();
  return rv;
};
