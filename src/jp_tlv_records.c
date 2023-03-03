
#include <apr_strings.h>

#include <json.h>
#include <json_visit.h>

#include "jp_tlv_encoder.h"


static json_object*
jp_process_segment(struct json_tokener *tok,
                  const char           *segment,
                  size_t                width)
{
  return json_tokener_parse_ex(tok, segment, width);
}


int jp_update_records_from_file(apr_pool_t       *pool,
                                jp_TLV_records_t *tlv_records,
                                FILE             *input)
{
  #define JP_FREAD_BUFFER_SIZE 4096
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