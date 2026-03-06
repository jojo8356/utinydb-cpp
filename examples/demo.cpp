/*
 * demo.cpp — UTinyDB (C++) usage example
 */

#include "utinydb.hpp"

#include <cstdio>
#include <filesystem>

using namespace utinydb;

int main(void)
{
    printf("=== UTinyDB (C++) Demo ===\n\n");

    /* Open database */
    auto db = Database::open("demo.json");
    auto& users = db->collection("users");
    auto& posts = db->collection("posts");

    /* Clear for fresh demo */
    users.clear();
    posts.clear();

    /* Insert users */
    Doc d;
    d.set_str("name", "Alice");
    d.set_int("age", 25);
    d.set_str("role", "admin");
    users.insert(d);

    d = Doc();
    d.set_str("name", "Bob");
    d.set_int("age", 30);
    d.set_str("role", "user");
    users.insert(d);

    d = Doc();
    d.set_str("name", "Charlie");
    d.set_int("age", 22);
    d.set_str("role", "user");
    users.insert(d);

    /* Insert posts */
    d = Doc();
    d.set_str("title", "Hello World");
    d.set_int("author_id", 1);
    posts.insert(d);

    d = Doc();
    d.set_str("title", "C++ Programming");
    d.set_int("author_id", 2);
    posts.insert(d);

    /* Find all users */
    printf("All users (%d):\n", users.count());
    for (auto *doc : users.find_all()) {
        printf("  [%d] %s, age %lld, role: %s\n",
               doc->id(),
               doc->get_str("name"),
               (long long)doc->get_int("age"),
               doc->get_str("role"));
    }

    /* Find admins */
    Doc q;
    q.set_str("role", "admin");
    auto admins = users.find(q);
    printf("\nAdmins (%d):\n", (int)admins.size());
    for (auto *doc : admins) {
        printf("  %s\n", doc->get_str("name"));
    }

    /* Update */
    Doc upd_q;
    upd_q.set_str("name", "Alice");
    Doc upd_data;
    upd_data.set_int("age", 26);
    users.update(upd_q, upd_data);

    Doc *alice = users.get(1);
    printf("\nAlice after update: age = %lld\n", (long long)alice->get_int("age"));

    /* Aggregations */
    printf("\nAggregations:\n");
    printf("  count:   %d\n", users.count());
    printf("  min age: %lld\n", (long long)users.min_int("age"));
    printf("  max age: %lld\n", (long long)users.max_int("age"));
    printf("  avg age: %.1f\n", users.avg("age"));

    /* Delete */
    users.remove_by_id(3);
    printf("\nAfter deleting Charlie: %d users\n", users.count());

    printf("\nFile saved to demo.json\n");

    /* Clean up demo file */
    std::filesystem::remove("demo.json");
    return 0;
}
