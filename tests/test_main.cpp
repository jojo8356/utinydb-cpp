#include "utest.h"
#include "utinydb.hpp"

/* Include all test files (single compilation unit) */
#include "test_doc.cpp"
#include "test_crud.cpp"
#include "test_aggregation.cpp"
#include "test_persistence.cpp"

int main(void)
{
    printf("UTinyDB (C++) — Full Test Suite\n");
    printf("========================================\n");

    /* Doc */
    printf("\n--- Doc ---\n");
    TEST(test_doc_types);
    TEST(test_doc_has);
    TEST(test_doc_clone);
    TEST(test_doc_remove_field);
    TEST(test_doc_overwrite);
    TEST(test_doc_default_values);

    /* CRUD */
    printf("\n--- CRUD ---\n");
    TEST(test_insert_simple);
    TEST(test_insert_auto_id);
    TEST(test_insert_many);
    TEST(test_find_all);
    TEST(test_find_by_field);
    TEST(test_find_one);
    TEST(test_get_by_id);
    TEST(test_update);
    TEST(test_update_by_id);
    TEST(test_delete);
    TEST(test_delete_by_id);
    TEST(test_clear);
    TEST(test_drop);

    /* Aggregations */
    printf("\n--- Aggregations ---\n");
    TEST(test_count);
    TEST(test_exists);
    TEST(test_min_max);
    TEST(test_avg);
    TEST(test_sum);

    /* Persistence */
    printf("\n--- Persistence ---\n");
    TEST(test_save_and_reload);
    TEST(test_auto_save);
    TEST(test_export);
    TEST(test_reload);
    TEST(test_in_memory);
    TEST(test_json_special_chars);
    TEST(test_null_values);

    TEST_REPORT();
    return TEST_EXIT_CODE();
}
