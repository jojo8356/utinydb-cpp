/*
 * test_doc.cpp — Doc tests (types, getters, setters)
 */

using namespace utinydb;

static void test_doc_types(void)
{
    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_int("age", 25);
    doc.set_dbl("score", 95.5);
    doc.set_bool("active", true);
    doc.set_null("email");

    ASSERT(strcmp(doc.get_str("name"), "Alice") == 0, "str getter");
    ASSERT_EQ((int)doc.get_int("age"), 25, "int getter");
    ASSERT(doc.get_dbl("score") > 95.0, "dbl getter");
    ASSERT_EQ((int)doc.get_bool("active"), 1, "bool getter");
    ASSERT_EQ((int)doc.get_type("email"), (int)ValType::Null, "null type");
}

static void test_doc_has(void)
{
    Doc doc;
    doc.set_str("name", "Alice");

    ASSERT_EQ((int)doc.has("name"), 1, "has name");
    ASSERT_EQ((int)doc.has("nope"), 0, "no nope");
}

static void test_doc_clone(void)
{
    auto db = Database::memory();
    auto& col = db->collection("test");

    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_int("age", 25);
    Doc& ins = col.insert(doc);

    Doc clone = ins.clone();
    ASSERT(strcmp(clone.get_str("name"), "Alice") == 0, "clone name");
    ASSERT_EQ((int)clone.get_int("age"), 25, "clone age");
    ASSERT_EQ(clone.id(), ins.id(), "clone id");

    /* Mutating clone doesn't affect original */
    clone.set_str("name", "Bob");
    ASSERT(strcmp(ins.get_str("name"), "Alice") == 0, "original unchanged");
}

static void test_doc_remove_field(void)
{
    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_int("age", 25);
    doc.set_str("role", "admin");

    ASSERT_EQ(doc.field_count(), 3, "3 fields");
    doc.remove_field("age");
    ASSERT_EQ(doc.field_count(), 2, "2 fields after remove");
    ASSERT_EQ((int)doc.has("age"), 0, "age removed");
    ASSERT_EQ((int)doc.has("name"), 1, "name still there");
    ASSERT_EQ((int)doc.has("role"), 1, "role still there");
}

static void test_doc_overwrite(void)
{
    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_str("name", "Bob");

    ASSERT(strcmp(doc.get_str("name"), "Bob") == 0, "overwrite works");
    ASSERT_EQ(doc.field_count(), 1, "still 1 field");
}

static void test_doc_default_values(void)
{
    Doc doc;

    ASSERT(doc.get_str("missing") == nullptr, "missing str is nullptr");
    ASSERT_EQ((int)doc.get_int("missing"), 0, "missing int is 0");
    ASSERT(doc.get_dbl("missing") == 0.0, "missing dbl is 0.0");
    ASSERT_EQ((int)doc.get_bool("missing"), 0, "missing bool is false");
}
