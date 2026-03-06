# UTinyDB (C++)

Lightweight JSON document database in C++17. Zero external dependencies — pure STL.

## Quick Start

```cpp
#include "utinydb.hpp"

using namespace utinydb;

int main()
{
    auto db = Database::open("data.json");
    auto& users = db->collection("users");

    // Insert
    Doc doc;
    doc.set_str("name", "Alice");
    doc.set_int("age", 25);
    doc.set_str("role", "admin");
    users.insert(doc);

    // Find by field
    Doc q;
    q.set_str("role", "admin");
    for (auto* user : users.find(q)) {
        printf("%s (age %lld)\n",
               user->get_str("name"),
               (long long)user->get_int("age"));
    }

    return 0;
}
```

The JSON file is always human-readable:

```json
{
  "users": [
    {"_id": 1, "name": "Alice", "age": 25, "role": "admin"}
  ]
}
```

## API

### Database

```cpp
auto db = Database::open("data.json");              // Open/create file
auto db = Database::open(path, pretty, auto_save);   // With options
auto db = Database::memory();                         // In-memory only
// ~Database() saves and frees automatically (RAII)
db->save();                                           // Manual save
db->reload();                                         // Reload from file
db->export_to("backup.json");                         // Export to file
```

### Collections

```cpp
auto& users = db->collection("users");
db->drop("users");
db->collection_count();
```

### Documents (Doc)

```cpp
Doc doc;

// Setters
doc.set_str("name", "Alice");
doc.set_int("age", 25);
doc.set_dbl("score", 95.5);
doc.set_bool("active", true);
doc.set_null("email");

// Getters (return default if missing)
const char* name = doc.get_str("name");  // nullptr if missing
int64_t age      = doc.get_int("age");   // 0 if missing
double score     = doc.get_dbl("score"); // 0.0 if missing
bool active      = doc.get_bool("active");

// Utilities
doc.has("name");         // true or false
doc.id();                // _id value
doc.field_count();
doc.remove_field("key");
Doc copy = doc.clone();
```

### CRUD

```cpp
// Insert (doc is cloned internally)
Doc& inserted = col.insert(doc);
col.insert_many(docs_vector);

// Find
Doc* doc = col.get(id);                  // By _id
Doc* doc = col.find_one(query);          // First match
auto results = col.find(query);          // All matches (vector<Doc*>)
auto all     = col.find_all();           // All docs
col.count();
col.exists(query);

// Update (merges data into matching docs)
col.update(query, data);
col.update_by_id(id, data);

// Delete
col.remove(query);
col.remove_by_id(id);
col.clear();
```

### Iterating Results

```cpp
auto results = col.find(query);
for (auto* doc : results) {
    printf("%s\n", doc->get_str("name"));
}
```

### Aggregations

```cpp
col.min_int("age");
col.max_int("age");
col.avg("age");
col.sum_int("score");
```

## Build

```bash
make test           # Build + run tests
make test-asan      # AddressSanitizer + UBSan
make test-valgrind  # Memory leak check
make demo           # Build and run example
make clean
```

## Architecture

```
utinydb-cpp/
+-- include/
|   +-- utinydb.hpp         # Public API header
+-- src/
|   +-- utinydb.cpp         # Core: Doc, Collection, Database
|   +-- utinydb_json.cpp    # JSON parser/writer
+-- tests/
|   +-- utest.h             # Mini test framework
|   +-- test_main.cpp       # Test runner
|   +-- test_doc.cpp        # Doc tests
|   +-- test_crud.cpp       # CRUD tests
|   +-- test_aggregation.cpp # Aggregation tests
|   +-- test_persistence.cpp # File I/O tests
+-- examples/
|   +-- demo.cpp            # Usage example
+-- Makefile
```

## C vs C++ API Comparison

| C | C++ |
|---|-----|
| `UTinyDB *db = utinydb_open(path)` | `auto db = Database::open(path)` |
| `UCollection *col = utinydb_collection(db, "users")` | `auto& col = db->collection("users")` |
| `UDoc *doc = udoc_new()` | `Doc doc;` |
| `udoc_set_str(doc, "k", "v")` | `doc.set_str("k", "v")` |
| `UVec *r = ucol_find(col, q)` | `auto r = col.find(q)` |
| `ucol_foreach(r, d) { ... }` | `for (auto* d : r) { ... }` |
| `uvec_free(r); udoc_free(doc); utinydb_close(db)` | RAII (automatic) |

## Specs

| Feature | Status |
|---|---|
| C++17, zero external deps (pure STL) | OK |
| JSON file storage (human-readable) | OK |
| CRUD (insert, find, update, delete) | OK |
| Query by field equality | OK |
| Auto-incrementing _id | OK |
| Multiple value types (string, int, double, bool, null) | OK |
| Auto-save mode | OK |
| File persistence via std::filesystem | OK |
| Aggregations (min, max, avg, sum) | OK |
| In-memory mode (no file) | OK |
| Export/reload | OK |
| Range-based for iteration | OK |
| RAII (no manual free) | OK |
| 31 tests passing | OK |

## Licence

MIT
