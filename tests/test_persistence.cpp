/*
 * test_persistence.cpp — JSON persistence tests
 */

#include <filesystem>

using namespace utinydb;
namespace fs = std::filesystem;

static void test_save_and_reload(void)
{
    const char *path = "/tmp/utinydb_cpp_test_save.json";
    fs::remove(path);

    /* Write */
    {
        auto db = Database::open(path, true, false);
        auto& users = db->collection("users");

        Doc d;
        d.set_str("name", "Alice");
        d.set_int("age", 25);
        d.set_dbl("score", 95.5);
        d.set_bool("active", true);
        users.insert(d);

        d = Doc();
        d.set_str("name", "Bob");
        d.set_int("age", 30);
        users.insert(d);

        db->save();
    }

    /* Read back */
    {
        auto db = Database::open(path);
        auto& users = db->collection("users");

        ASSERT_EQ(users.count(), 2, "reloaded 2 docs");

        Doc *alice = users.get(1);
        ASSERT_NOT_NULL(alice, "alice loaded");
        ASSERT(strcmp(alice->get_str("name"), "Alice") == 0, "name Alice");
        ASSERT_EQ((int)alice->get_int("age"), 25, "age 25");
        ASSERT(alice->get_dbl("score") > 95.0, "score 95.5");
        ASSERT_EQ((int)alice->get_bool("active"), 1, "active true");

        Doc *bob = users.get(2);
        ASSERT_NOT_NULL(bob, "bob loaded");
        ASSERT(strcmp(bob->get_str("name"), "Bob") == 0, "name Bob");
    }

    fs::remove(path);
}

static void test_auto_save(void)
{
    const char *path = "/tmp/utinydb_cpp_test_autosave.json";
    fs::remove(path);

    {
        auto db = Database::open(path, true, true); /* auto_save=true */
        auto& users = db->collection("users");

        Doc d;
        d.set_str("name", "Alice");
        users.insert(d);

        /* File should exist now (auto-saved) */
        ASSERT(fs::exists(path), "file created by auto_save");
    }

    /* Verify content */
    {
        auto db = Database::open(path);
        auto& users = db->collection("users");
        ASSERT_EQ(users.count(), 1, "auto-saved 1 doc");
    }

    fs::remove(path);
}

static void test_export(void)
{
    const char *path = "/tmp/utinydb_cpp_test_export.json";
    fs::remove(path);

    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    users.insert(d);

    bool ok = db->export_to(path);
    ASSERT_EQ((int)ok, 1, "export succeeded");
    ASSERT(fs::exists(path), "export file exists");

    /* Verify */
    {
        auto db2 = Database::open(path);
        auto& users2 = db2->collection("users");
        ASSERT_EQ(users2.count(), 1, "exported 1 doc");
    }

    fs::remove(path);
}

static void test_reload(void)
{
    const char *path = "/tmp/utinydb_cpp_test_reload.json";
    fs::remove(path);

    {
        auto db = Database::open(path, true, false);
        auto& users = db->collection("users");

        Doc d;
        d.set_str("name", "Alice");
        users.insert(d);
        db->save();

        /* Modify via another db instance */
        {
            auto db2 = Database::open(path, true, false);
            auto& u2 = db2->collection("users");
            Doc d2;
            d2.set_str("name", "Bob");
            u2.insert(d2);
            db2->save();
        }

        /* Reload original */
        db->reload();
        auto& users2 = db->collection("users");
        ASSERT_EQ(users2.count(), 2, "reloaded 2 docs");
    }

    fs::remove(path);
}

static void test_in_memory(void)
{
    auto db = Database::memory();
    auto& col = db->collection("test");

    Doc d;
    d.set_int("x", 42);
    col.insert(d);

    ASSERT_EQ(col.count(), 1, "in-memory works");
}

static void test_json_special_chars(void)
{
    const char *path = "/tmp/utinydb_cpp_test_special.json";
    fs::remove(path);

    {
        auto db = Database::open(path, true, false);
        auto& col = db->collection("data");
        Doc d;
        d.set_str("text", "line1\nline2\ttab \"quoted\" back\\slash");
        col.insert(d);
        db->save();
    }

    {
        auto db = Database::open(path);
        auto& col = db->collection("data");
        Doc *doc = col.get(1);
        ASSERT_NOT_NULL(doc, "special chars doc loaded");
        const char *text = doc->get_str("text");
        ASSERT_NOT_NULL(text, "text field exists");
        ASSERT(strstr(text, "line1\nline2") != nullptr, "newline preserved");
        ASSERT(strstr(text, "\"quoted\"") != nullptr, "quotes preserved");
        ASSERT(strstr(text, "back\\slash") != nullptr, "backslash preserved");
    }

    fs::remove(path);
}

static void test_null_values(void)
{
    const char *path = "/tmp/utinydb_cpp_test_null.json";
    fs::remove(path);

    {
        auto db = Database::open(path, true, false);
        auto& col = db->collection("data");
        Doc d;
        d.set_str("name", "Alice");
        d.set_null("email");
        col.insert(d);
        db->save();
    }

    {
        auto db = Database::open(path);
        auto& col = db->collection("data");
        Doc *doc = col.get(1);
        ASSERT_NOT_NULL(doc, "null doc loaded");
        ASSERT_EQ((int)doc->get_type("email"), (int)ValType::Null,
                  "email is null");
        ASSERT(doc->has("email"), "email field exists");
    }

    fs::remove(path);
}
