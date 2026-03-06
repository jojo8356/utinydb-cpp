/*
 * demo.cpp — UTinyDB (C++) usage example
 */

#include "utinydb.hpp"

#include <prism/prism.hpp>
#include <filesystem>

using namespace utinydb;
using namespace prism;

int main(void)
{
    println("=== UTinyDB (C++) Demo ===\n");

    {
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
        println("All users ({}):", users.count());
        for (auto *doc : users.find_all()) {
            println("  [{}] {}, age {}, role: {}",
                    doc->id(),
                    doc->get_str("name"),
                    doc->get_int("age"),
                    doc->get_str("role"));
        }

        /* Find admins */
        Doc q;
        q.set_str("role", "admin");
        auto admins = users.find(q);
        println("\nAdmins ({}):", admins.size());
        for (auto *doc : admins) {
            println("  {}", doc->get_str("name"));
        }

        /* Update */
        Doc upd_q;
        upd_q.set_str("name", "Alice");
        Doc upd_data;
        upd_data.set_int("age", 26);
        users.update(upd_q, upd_data);

        Doc *alice = users.get(1);
        println("\nAlice after update: age =", alice->get_int("age"));

        /* Aggregations */
        println("\nAggregations:");
        println("  count:  ", users.count());
        println("  min age:", users.min_int("age"));
        println("  max age:", users.max_int("age"));
        println("  avg age:", users.avg("age"));

        /* Delete */
        users.remove_by_id(3);
        println("\nAfter deleting Charlie:", users.count(), "users");

        println("\nFile saved to demo.json");
    }

    /* Clean up demo file (after db is destroyed) */
    std::filesystem::remove("demo.json");
    return 0;
}
