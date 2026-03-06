/*
 * test_crud.cpp — CRUD tests for UTinyDB
 */

using namespace utinydb;

/* ---- Insert ---- */

static void test_insert_simple(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_int("age", 25);
    Doc& result = users.insert(doc);

    ASSERT_EQ(result.id(), 1, "first id is 1");
    ASSERT_EQ(users.count(), 1, "count is 1");
}

static void test_insert_auto_id(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d1;
    d1.set_str("name", "A");
    Doc& r1 = users.insert(d1);

    Doc d2;
    d2.set_str("name", "B");
    Doc& r2 = users.insert(d2);

    ASSERT(r1.id() != r2.id(), "distinct ids");
    ASSERT_EQ(users.count(), 2, "count is 2");
}

static void test_insert_many(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d1; d1.set_str("name", "A");
    Doc d2; d2.set_str("name", "B");
    Doc d3; d3.set_str("name", "C");
    std::vector<Doc> docs = {d1, d2, d3};
    int n = users.insert_many(docs);

    ASSERT_EQ(n, 3, "inserted 3");
    ASSERT_EQ(users.count(), 3, "count is 3");
}

/* ---- Find ---- */

static void test_find_all(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d; d.set_str("name", "A"); users.insert(d);
    d = Doc(); d.set_str("name", "B"); users.insert(d);

    auto all = users.find_all();
    ASSERT_EQ((int)all.size(), 2, "find_all returns 2");
}

static void test_find_by_field(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    d.set_str("role", "admin");
    users.insert(d);

    d = Doc();
    d.set_str("name", "Bob");
    d.set_str("role", "user");
    users.insert(d);

    Doc q;
    q.set_str("role", "admin");
    auto results = users.find(q);
    ASSERT_EQ((int)results.size(), 1, "found 1 admin");
    ASSERT(strcmp(results[0]->get_str("name"), "Alice") == 0, "found Alice");
}

static void test_find_one(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    users.insert(d);

    Doc q;
    q.set_str("name", "Alice");
    Doc *found = users.find_one(q);
    ASSERT_NOT_NULL(found, "found Alice");

    q.set_str("name", "ZZZ");
    Doc *miss = users.find_one(q);
    ASSERT(miss == nullptr, "ZZZ not found");
}

static void test_get_by_id(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    Doc& ins = users.insert(d);
    int id = ins.id();

    Doc *got = users.get(id);
    ASSERT_NOT_NULL(got, "get by id");
    ASSERT(strcmp(got->get_str("name"), "Alice") == 0, "correct doc");
    ASSERT(users.get(999) == nullptr, "get absent returns nullptr");
}

/* ---- Update ---- */

static void test_update(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    d.set_int("age", 25);
    Doc& ins = users.insert(d);

    Doc q;
    q.set_str("name", "Alice");
    Doc data;
    data.set_int("age", 26);

    int n = users.update(q, data);
    ASSERT_EQ(n, 1, "updated 1");
    ASSERT_EQ((int)users.get(ins.id())->get_int("age"), 26, "age is 26");
}

static void test_update_by_id(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    d.set_int("age", 25);
    Doc& ins = users.insert(d);

    Doc data;
    data.set_int("age", 30);
    int ok = users.update_by_id(ins.id(), data);
    ASSERT_EQ(ok, 1, "updated");
    ASSERT_EQ((int)users.get(ins.id())->get_int("age"), 30, "age=30");
}

/* ---- Delete ---- */

static void test_delete(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    users.insert(d);

    d = Doc();
    d.set_str("name", "Bob");
    users.insert(d);

    Doc q;
    q.set_str("name", "Alice");
    int n = users.remove(q);
    ASSERT_EQ(n, 1, "deleted 1");
    ASSERT_EQ(users.count(), 1, "1 remaining");
}

static void test_delete_by_id(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    Doc& ins = users.insert(d);

    int ok = users.remove_by_id(ins.id());
    ASSERT_EQ(ok, 1, "deleted");
    ASSERT_EQ(users.count(), 0, "empty");
}

static void test_clear(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d; d.set_str("name", "A"); users.insert(d);
    d = Doc(); d.set_str("name", "B"); users.insert(d);

    users.clear();
    ASSERT_EQ(users.count(), 0, "cleared");
}

static void test_drop(void)
{
    auto db = Database::memory();
    db->collection("users");
    ASSERT_EQ(db->collection_count(), 1, "1 collection");
    db->drop("users");
    ASSERT_EQ(db->collection_count(), 0, "0 collections");
}
