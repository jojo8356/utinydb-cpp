/*
 * test_aggregation.cpp — Aggregation tests
 */

using namespace utinydb;

static void test_count(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    ASSERT_EQ(users.count(), 0, "empty count");

    Doc d; d.set_str("name", "A"); users.insert(d);
    d = Doc(); d.set_str("name", "B"); users.insert(d);

    ASSERT_EQ(users.count(), 2, "count 2");
}

static void test_exists(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "Alice");
    users.insert(d);

    Doc q;
    q.set_str("name", "Alice");
    ASSERT_EQ((int)users.exists(q), 1, "exists Alice");

    q.set_str("name", "ZZZ");
    ASSERT_EQ((int)users.exists(q), 0, "not exists ZZZ");
}

static void test_min_max(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_str("name", "A"); d.set_int("age", 25); users.insert(d);
    d = Doc(); d.set_str("name", "B"); d.set_int("age", 30); users.insert(d);
    d = Doc(); d.set_str("name", "C"); d.set_int("age", 22); users.insert(d);

    ASSERT_EQ((int)users.min_int("age"), 22, "min age 22");
    ASSERT_EQ((int)users.max_int("age"), 30, "max age 30");
}

static void test_avg(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_int("age", 20); users.insert(d);
    d = Doc(); d.set_int("age", 30); users.insert(d);
    d = Doc(); d.set_int("age", 40); users.insert(d);

    double avg = users.avg("age");
    ASSERT(avg > 29.9 && avg < 30.1, "avg age ~30");
}

static void test_sum(void)
{
    auto db = Database::memory();
    auto& users = db->collection("users");

    Doc d;
    d.set_int("score", 10); users.insert(d);
    d = Doc(); d.set_int("score", 20); users.insert(d);
    d = Doc(); d.set_int("score", 30); users.insert(d);

    ASSERT_EQ((int)users.sum_int("score"), 60, "sum 60");
}
