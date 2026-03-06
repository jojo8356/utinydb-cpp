/*
 * UTinyDB — Universal TinyDB (C++)
 * Lightweight JSON document database
 *
 * Zero external dependencies — pure C++17 STL.
 *
 * Usage:
 *   #include "utinydb.hpp"
 *
 *   auto db = utinydb::Database::open("data.json");
 *   auto& users = db->collection("users");
 *
 *   utinydb::Doc doc;
 *   doc.set_str("name", "Alice");
 *   doc.set_int("age", 25);
 *   users.insert(doc);
 */

#ifndef UTINYDB_HPP
#define UTINYDB_HPP

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <cstdint>

namespace utinydb {

/* ============================================================
 * Value types
 * ============================================================ */

enum class ValType { Null = 0, Bool, Int, Double, String };

using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

/* Forward declarations */
class Collection;
class Database;

/* ============================================================
 * Doc — Document (key-value pairs)
 * ============================================================ */

class Doc {
public:
    Doc() = default;

    /* Setters */
    void set_str(const std::string& key, const std::string& val);
    void set_int(const std::string& key, int64_t val);
    void set_dbl(const std::string& key, double val);
    void set_bool(const std::string& key, bool val);
    void set_null(const std::string& key);

    /* Getters (return default if missing: nullptr/0/0.0/false) */
    const char *get_str(const std::string& key) const;
    int64_t     get_int(const std::string& key) const;
    double      get_dbl(const std::string& key) const;
    bool        get_bool(const std::string& key) const;
    ValType     get_type(const std::string& key) const;

    /* Utilities */
    bool has(const std::string& key) const;
    int  id() const { return id_; }
    int  field_count() const { return static_cast<int>(fields_.size()); }
    void remove_field(const std::string& key);
    Doc  clone() const;

    /* Field access (for JSON serialization) */
    const std::vector<std::pair<std::string, Value>> &fields() const {
        return fields_;
    }

    /* Sync _id member from "_id" field (used by JSON parser) */
    void sync_id_from_field() {
        if (has("_id")) id_ = static_cast<int>(get_int("_id"));
    }

private:
    std::vector<std::pair<std::string, Value>> fields_;
    int id_ = -1;

    Value       *find_val(const std::string& key);
    const Value *find_val(const std::string& key) const;
    void         set_value(const std::string& key, Value val);

    friend class Collection;
};

/* ============================================================
 * Collection — Named collection of documents
 * ============================================================ */

class Collection {
public:
    /* Insert — doc is cloned internally */
    Doc &insert(const Doc& doc);
    int  insert_many(const std::vector<Doc>& docs);

    /* Find */
    Doc              *get(int id);
    Doc              *find_one(const Doc& query);
    std::vector<Doc*> find(const Doc& query);
    std::vector<Doc*> find_all();
    int               count() const { return static_cast<int>(docs_.size()); }
    bool              exists(const Doc& query);

    /* Update — merges data fields into matching docs */
    int update(const Doc& query, const Doc& data);
    int update_by_id(int id, const Doc& data);

    /* Delete */
    int  remove(const Doc& query);
    int  remove_by_id(int id);
    void clear();

    /* Aggregations */
    int64_t min_int(const std::string& field);
    int64_t max_int(const std::string& field);
    double  avg(const std::string& field);
    int64_t sum_int(const std::string& field);

    const std::string &name() const { return name_; }

    /* Add a pre-built doc directly, preserving its _id (used by JSON parser) */
    void add_doc_raw(std::unique_ptr<Doc> doc) {
        if (doc->id() >= next_id_) next_id_ = doc->id() + 1;
        docs_.push_back(std::move(doc));
    }

    /* Read-only doc access (for JSON writer) */
    int doc_count() const { return static_cast<int>(docs_.size()); }
    const Doc &doc_at(int i) const { return *docs_[static_cast<size_t>(i)]; }

private:
    Collection(const std::string& name, Database* db);

    std::string                       name_;
    std::vector<std::unique_ptr<Doc>> docs_;
    int                               next_id_ = 1;
    Database                         *db_;

    static bool matches(const Doc& doc, const Doc& query);
    static void merge(Doc& target, const Doc& data);
    void auto_save();

    friend class Database;
};

/* ============================================================
 * Database — Main database
 * ============================================================ */

class Database {
public:
    ~Database();

    /* Factory methods */
    static std::unique_ptr<Database> open(const std::string& path);
    static std::unique_ptr<Database> open(const std::string& path,
                                          bool pretty, bool auto_save);
    static std::unique_ptr<Database> memory();

    /* Non-copyable, non-movable (pointer stability for back-refs) */
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = delete;
    Database& operator=(Database&&) = delete;

    /* Collection access */
    Collection &collection(const std::string& name);
    void        drop(const std::string& name);
    int         collection_count() const {
        return static_cast<int>(collections_.size());
    }

    /* Persistence */
    void save();
    void reload();
    bool export_to(const std::string& path);

    bool auto_save_enabled() const { return auto_save_; }
    bool pretty() const { return pretty_; }

    /* Read-only collection access (for JSON writer) */
    const Collection &collection_at(int i) const {
        return *collections_[static_cast<size_t>(i)];
    }

private:
    Database() = default;

    std::string                              path_;
    std::vector<std::unique_ptr<Collection>> collections_;
    bool                                     auto_save_ = true;
    bool                                     pretty_ = true;

    void load_from_file();
};

/* Internal JSON functions (separate compilation unit) */
bool        detail_json_parse(Database& db, const std::string& json);
std::string detail_json_write(const Database& db, bool pretty);

} /* namespace utinydb */

#endif /* UTINYDB_HPP */
