
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

static int json_visitor(json_object *jso,
                        int          flags,
                        json_object *parent_jso,
                        const char  *jso_key,
                        size_t      *jso_index,
                        void        *userarg)
{
    if (flags == JSON_C_VISIT_SECOND)
      return JSON_C_VISIT_RETURN_CONTINUE;

    enum json_type type = json_object_get_type(jso);

    printf("flags: 0x%x, type: %d  key: %s, index: %ld, value: %s\n", flags, type,
	       (jso_key ? jso_key : "(null)"), (jso_index ? (long)*jso_index : -1L),
	       json_object_to_json_string(jso));

    return JSON_C_VISIT_RETURN_CONTINUE;
}


int jp_process_file(apr_pool_t          *pool,
                    jp_TLV_records_t    *tlv_records,
                    FILE                *input)
{
  #define JP_FREAD_BUFFER_SIZE 4096
	char                    buffer[JP_FREAD_BUFFER_SIZE];
  char                   *next_line        = NULL;
  json_object*            next_line_object = NULL;
  struct json_tokener    *tokener = json_tokener_new();
  enum json_tokener_error jerr;
	int                     ret = 0;
	int32_t                 read;
  int32_t                 line_size = 0;
	int                     i;
	while (1) {
    uint32_t last_parsed_offset = 0, next_segment_size_to_parse = 0;
		read = fread(buffer, 1, JP_FREAD_BUFFER_SIZE, input);

		if (read <= 0)
			break;

		for (i = 0; i < read; i++) {
      int is_last               = (i == read - 1);
      int is_terminal_character = (buffer[i] == '\n' || buffer[i] == '\0');

			if (is_terminal_character || is_last) {
        if (next_line == NULL) {
          next_line = malloc( next_segment_size_to_parse + 1 );
          memcpy(next_line, buffer + last_parsed_offset, next_segment_size_to_parse + 1);
          line_size = next_segment_size_to_parse;
        } else {
          next_line  = realloc(next_line, next_segment_size_to_parse + 1);
          memcpy(next_line + line_size, buffer + last_parsed_offset, next_segment_size_to_parse + 1);
          line_size += next_segment_size_to_parse;
        }

        next_segment_size_to_parse = 0;

        if (!is_last || is_terminal_character) {
          next_line_object   = jp_process_segment(tokener, next_line, line_size + 1);
          last_parsed_offset = i + 1;
          free(next_line);
          next_line = NULL;
          line_size = 0;
        }

        if (next_line_object)
        {
          jp_update_records_from_json(pool, tlv_records, next_line_object);
          json_tokener_reset(tokener);
          json_object_put(next_line_object);
          next_line_object = NULL;
        }
        else if ((jerr = json_tokener_get_error(tokener)) != json_tokener_continue)
        {
        	fprintf(stderr, "JSON Tokener Error: %s\n", json_tokener_error_desc(jerr));
          json_tokener_free(tokener);
          return 1;
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

  apr_terminate();
  return rv;
};
