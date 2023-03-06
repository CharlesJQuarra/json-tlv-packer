
#include <apr.h>

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include <jp_tlv_encoder.h>

#define BUFFER_SIZE 1024

static
apr_pool_t *pool;

void setup() {
  apr_pool_create(&pool, NULL);
}

void teardown() {
  apr_pool_destroy(pool);
}

static
int compare_kv_pairs(const jp_TLV_kv_pair_t* pair_A, const jp_TLV_kv_pair_t* pair_B) {
  if (pair_A->key_index != pair_B->key_index)
    return -1;

  if (pair_A->value_type != pair_B->value_type)
    return -1;

  int boolean_A, boolean_B;
  int32_t integer_A, integer_B;
  double double_A, double_B;
  const char *string_A, *string_B;

  switch (pair_A->value_type) {
    case JP_TYPE_BOOLEAN:
    jp_read_boolean_from_kv_pair(pair_A, & boolean_A);
    jp_read_boolean_from_kv_pair(pair_B, & boolean_B);
    return !(boolean_A == boolean_B);

    case JP_TYPE_INTEGER:
    jp_read_integer_from_kv_pair(pair_A, & integer_A);
    jp_read_integer_from_kv_pair(pair_B, & integer_B);
    return !(integer_A == integer_B);

    case JP_TYPE_DOUBLE:
    jp_read_double_from_kv_pair(pair_A, & double_A);
    jp_read_double_from_kv_pair(pair_B, & double_B);
    return !(double_A == double_B);

    case JP_TYPE_STRING:
    jp_read_string_from_kv_pair(pair_A, & string_A);
    jp_read_string_from_kv_pair(pair_B, & string_B);
    return strcmp(string_A, string_B);

    case default:
    return -1;
  }

  return -1;
}

static
void test_kv_encoding(const jp_TLV_record_t* record, char *out_buffer, size_t buffer_size) {
  /* act */
        uint32_t            written  = 0;
        uint32_t            read     = 0;
  const apr_array_header_t* kv_array = record->kv_pairs_array;
        uint32_t            nb_pairs = kv_array->nelts;

  for (int i = 0; i < nb_pairs; i++) {
    jp_TLV_kv_pair_t* elem = & ((jp_TLV_kv_pair_t*)kv_array->elts)[i];

    ck_assert_msg(buffer_size > written, "buffer size too small");
    uint32_t new_written = jp_export_kv_pair_to_buffer(elem, out_buffer + written, buffer_size - written);

    ck_assert_msg (new_written > 0, "unable to write a kv pair to buffer");

    written += new_written;
  }

  /* check */

  for (int i = 0; i < nb_pairs; i++) {
    jp_TLV_kv_pair_t imported_elem;
    //memset(& imported_elem, 0, sizeof(jp_TLV_kv_pair_t));

    ck_assert_msg(buffer_size > read, "buffer size too small");
    uint32_t new_read = jp_import_kv_pair_from_buffer(pool, & imported_elem, out_buffer + read, buffer_size - read);

    ck_assert_msg (new_read > 0, "unable to read a kv pair from buffer");

    read += new_read;

    jp_TLV_kv_pair_t* expected_elem = & ((jp_TLV_kv_pair_t*)kv_array->elts)[i];

    /*
    printf("test_encoding: expected key_index: %d imported key index: %d \n", expected_elem->key_index, imported_elem.key_index);
    printf("test_encoding: expected type: %d imported type: %d \n", expected_elem->value_type, imported_elem.value_type);
    printf("test_encoding: expected value: %d imported value: %d \n", expected_elem->union_v.integer_value, imported_elem.union_v.integer_value);
    */
    ck_assert_msg(0 == compare_kv_pairs(expected_elem, & imported_elem, "key value pairs do not match");
  }

  printf("test_kv_encoding: bytes read: %d bytes written %d \n", read, written);
  ck_assert_msg(read == written, "read and written bytes do not match");
}



START_TEST(test_Boolean_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  jp_add_boolean_kv_pair_to_record(record, 1, 1);

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}

START_TEST(test_small_integer_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  jp_add_integer_kv_pair_to_record(record, 1, 31);
  printf("test_small_integer_KV_Pair_Encoding: encoded value 31\n");

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}
END_TEST

START_TEST(test_large_integer_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  jp_add_integer_kv_pair_to_record(record, 1, 33);
  printf("test_large_integer_KV_Pair_Encoding: encoded value 33\n");

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}
END_TEST

START_TEST(test_short_string_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  const char* value = "a short string";
  ck_assert_msg(strlen(value) < 32, " test string too long");
  jp_add_string_kv_pair_to_record(record, 1, value);

  printf("test_short_string_KV_Pair_Encoding: encoded value '%s'\n", value);

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}
END_TEST

START_TEST(test_long_string_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  const char* value = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";
  ck_assert_msg(strlen(value) > 31, " test string not long enough");

  jp_add_string_kv_pair_to_record(record, 1, value);

  printf("test_long_string_KV_Pair_Encoding: encoded value '%s'\n", value);

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}
END_TEST

START_TEST(test_composite_KV_Pair_Encoding)
{
  /* arrange */
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  const char* value1 = "a short string";
  ck_assert_msg(strlen(value1) < 32, " test string too long");
  jp_add_string_kv_pair_to_record(record, 1, value1);

  printf("test_composite_KV_Pair_Encoding: encoded 1st value: '%s'\n", value1);

  double value2 = 3.1416;
  jp_add_double_kv_pair_to_record(record, 18, value2);

  printf("test_composite_KV_Pair_Encoding: encoded 2nd value: %f\n", value2);

  int32_t value3 = -18;
  jp_add_integer_kv_pair_to_record(record, 517, value3);

  printf("test_composite_KV_Pair_Encoding: encoded 3rd value: %d\n", value3);

  test_kv_encoding(record, out_buffer, BUFFER_SIZE);
}
END_TEST


int compare_records(jp_TLV_record_t* record_A, jp_TLV_record_t* B) {

  apr_array_header_t* kv_array_A = record_A->kv_pairs_array;
  apr_array_header_t* kv_array_B = record_B->kv_pairs_array;

  uint32_t nb_pairs_A = kv_array_A->nelts;
  uint32_t nb_pairs_B = kv_array_B->nelts;

  if (nb_pairs_A != nb_pairs_B)
    return -1;

  for (int i = 0; i < nb_pairs_A; i++) {
    jp_TLV_kv_pair_t* elem_A = & ((jp_TLV_kv_pair_t*)kv_array_A->elts)[i];
    jp_TLV_kv_pair_t* elem_B = & ((jp_TLV_kv_pair_t*)kv_array_B->elts)[i];

    if (0 != compare_kv_pairs(elem_A, elem_B))
      return -1;
  }

  return 0;
}

START_TEST(test_TLV_record_export_import)
{
  /* arrange */
  jp_TLV_record_t* imported_record;
  jp_TLV_record_t* record = jp_TLV_record_make(pool);
  char out_buffer[BUFFER_SIZE];

  const char* value1 = "a short string";
  ck_assert_msg(strlen(value1) < 32, " test string too long");
  jp_add_string_kv_pair_to_record(record, 1, value1);

  printf("test_composite_KV_Pair_Encoding: encoded 1st value: '%s'\n", value1);

  double value2 = 3.1416;
  jp_add_double_kv_pair_to_record(record, 18, value2);

  printf("test_composite_KV_Pair_Encoding: encoded 2nd value: %f\n", value2);

  int32_t value3 = -18;
  jp_add_integer_kv_pair_to_record(record, 517, value3);

  printf("test_composite_KV_Pair_Encoding: encoded 3rd value: %d\n", value3);

  uint32_t written = jp_export_record_to_buffer(record, out_buffer, BUFFER_SIZE);
  uint32_t read    = jp_import_record_from_buffer(pool, & imported_record, out_buffer, BUFFER_SIZE);

  ck_assert_msg( 0 == compare_records(record, imported_record), "records do not match");
}
END_TEST


Suite * kv_pair_encoding_suite()
{
    Suite *s;
    TCase *tc_limits;

    s = suite_create("kv_pair_encoding");

    /* Core test case */
    TCase * tc_core_kv_encoding = tcase_create("Core");

    tcase_add_checked_fixture(tc_core_kv_encoding, setup, teardown);

    tcase_add_test(tc_core_kv_encoding, test_Boolean_KV_Pair_Encoding);
    tcase_add_test(tc_core_kv_encoding, test_small_integer_KV_Pair_Encoding);
    tcase_add_test(tc_core_kv_encoding, test_large_integer_KV_Pair_Encoding);
    tcase_add_test(tc_core_kv_encoding, test_short_string_KV_Pair_Encoding);
    tcase_add_test(tc_core_kv_encoding, test_long_string_KV_Pair_Encoding);
    tcase_add_test(tc_core_kv_encoding, test_composite_KV_Pair_Encoding);

    suite_add_tcase(s, tc_core_kv_encoding);

    /* Limits test case
    tc_limits = tcase_create("Limits");

    tcase_add_test(tc_limits, test_money_create_neg);
    tcase_add_test(tc_limits, test_money_create_zero);
    suite_add_tcase(s, tc_limits); */

    return s;
}

int main(int                argc,
         const char* const *argv)
{
    apr_status_t rv;
    pool = NULL;

    apr_app_initialize(&argc, &argv, NULL);
    atexit(apr_terminate);

    int number_failed;

    Suite *s    = kv_pair_encoding_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    apr_terminate();

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}