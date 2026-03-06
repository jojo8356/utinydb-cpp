/*
 * utinydb.cpp — Core implementation: Doc, Collection, Database
 */

#include "utinydb.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace utinydb {

/* ============================================================
 * Doc — Document
 * ============================================================ */

Value *Doc::find_val(const std::string& key)
{
    for (auto& [k, v] : fields_)
        if (k == key) return &v;
    return nullptr;
}

const Value *Doc::find_val(const std::string& key) const
{
    for (const auto& [k, v] : fields_)
        if (k == key) return &v;
    return nullptr;
}

void Doc::set_value(const std::string& key, Value val)
{
    if (auto *v = find_val(key)) {
        *v = std::move(val);
    } else {
        fields_.emplace_back(key, std::move(val));
    }
}

/* Setters */

void Doc::set_str(const std::string& key, const std::string& val)
{
    set_value(key, val);
}

void Doc::set_int(const std::string& key, int64_t val)
{
    set_value(key, val);
}

void Doc::set_dbl(const std::string& key, double val)
{
    set_value(key, val);
}

void Doc::set_bool(const std::string& key, bool val)
{
    set_value(key, val);
}

void Doc::set_null(const std::string& key)
{
    set_value(key, nullptr);
}

/* Getters */

const char *Doc::get_str(const std::string& key) const
{
    const auto *v = find_val(key);
    if (v && std::holds_alternative<std::string>(*v))
        return std::get<std::string>(*v).c_str();
    return nullptr;
}

int64_t Doc::get_int(const std::string& key) const
{
    const auto *v = find_val(key);
    if (!v) return 0;
    if (std::holds_alternative<int64_t>(*v))
        return std::get<int64_t>(*v);
    if (std::holds_alternative<double>(*v))
        return static_cast<int64_t>(std::get<double>(*v));
    if (std::holds_alternative<bool>(*v))
        return std::get<bool>(*v) ? 1 : 0;
    return 0;
}

double Doc::get_dbl(const std::string& key) const
{
    const auto *v = find_val(key);
    if (!v) return 0.0;
    if (std::holds_alternative<double>(*v))
        return std::get<double>(*v);
    if (std::holds_alternative<int64_t>(*v))
        return static_cast<double>(std::get<int64_t>(*v));
    return 0.0;
}

bool Doc::get_bool(const std::string& key) const
{
    const auto *v = find_val(key);
    if (v && std::holds_alternative<bool>(*v))
        return std::get<bool>(*v);
    return false;
}

ValType Doc::get_type(const std::string& key) const
{
    const auto *v = find_val(key);
    if (!v) return ValType::Null;
    if (std::holds_alternative<std::nullptr_t>(*v)) return ValType::Null;
    if (std::holds_alternative<bool>(*v))           return ValType::Bool;
    if (std::holds_alternative<int64_t>(*v))        return ValType::Int;
    if (std::holds_alternative<double>(*v))         return ValType::Double;
    if (std::holds_alternative<std::string>(*v))    return ValType::String;
    return ValType::Null;
}

/* Utilities */

bool Doc::has(const std::string& key) const
{
    return find_val(key) != nullptr;
}

void Doc::remove_field(const std::string& key)
{
    fields_.erase(
        std::remove_if(fields_.begin(), fields_.end(),
            [&key](const auto& p) { return p.first == key; }),
        fields_.end());
}

Doc Doc::clone() const
{
    Doc out;
    out.fields_ = fields_;
    out.id_ = id_;
    return out;
}

/* ============================================================
 * Collection
 * ============================================================ */

Collection::Collection(const std::string& name, Database *db)
    : name_(name), db_(db)
{
}

/* Insert */

Doc &Collection::insert(const Doc& doc)
{
    auto clone = std::make_unique<Doc>(doc.clone());
    clone->id_ = next_id_++;
    clone->set_int("_id", clone->id_);
    docs_.push_back(std::move(clone));
    auto_save();
    return *docs_.back();
}

int Collection::insert_many(const std::vector<Doc>& docs)
{
    int n = 0;
    for (const auto& doc : docs) {
        insert(doc);
        n++;
    }
    return n;
}

/* Find */

Doc *Collection::get(int id)
{
    for (auto& doc : docs_)
        if (doc->id_ == id) return doc.get();
    return nullptr;
}

Doc *Collection::find_one(const Doc& query)
{
    for (auto& doc : docs_)
        if (matches(*doc, query)) return doc.get();
    return nullptr;
}

std::vector<Doc*> Collection::find(const Doc& query)
{
    std::vector<Doc*> results;
    for (auto& doc : docs_)
        if (matches(*doc, query))
            results.push_back(doc.get());
    return results;
}

std::vector<Doc*> Collection::find_all()
{
    std::vector<Doc*> results;
    for (auto& doc : docs_)
        results.push_back(doc.get());
    return results;
}

bool Collection::exists(const Doc& query)
{
    return find_one(query) != nullptr;
}

/* Update */

int Collection::update(const Doc& query, const Doc& data)
{
    int updated = 0;
    for (auto& doc : docs_) {
        if (matches(*doc, query)) {
            merge(*doc, data);
            updated++;
        }
    }
    if (updated > 0) auto_save();
    return updated;
}

int Collection::update_by_id(int id, const Doc& data)
{
    Doc *doc = get(id);
    if (!doc) return 0;
    merge(*doc, data);
    auto_save();
    return 1;
}

/* Delete */

int Collection::remove(const Doc& query)
{
    int deleted = 0;
    auto it = docs_.begin();
    while (it != docs_.end()) {
        if (matches(**it, query)) {
            it = docs_.erase(it);
            deleted++;
        } else {
            ++it;
        }
    }
    if (deleted > 0) auto_save();
    return deleted;
}

int Collection::remove_by_id(int id)
{
    auto it = std::find_if(docs_.begin(), docs_.end(),
        [id](const auto& doc) { return doc->id_ == id; });
    if (it == docs_.end()) return 0;
    docs_.erase(it);
    auto_save();
    return 1;
}

void Collection::clear()
{
    docs_.clear();
    auto_save();
}

/* Matching / Merging */

bool Collection::matches(const Doc& doc, const Doc& query)
{
    if (query.fields_.empty()) return true;
    for (const auto& [key, val] : query.fields_) {
        const auto *dv = doc.find_val(key);
        if (!dv) return false;
        if (*dv != val) return false;
    }
    return true;
}

void Collection::merge(Doc& target, const Doc& data)
{
    for (const auto& [key, val] : data.fields_) {
        if (key == "_id") continue;
        target.set_value(key, val);
    }
}

void Collection::auto_save()
{
    if (db_ && db_->auto_save_enabled())
        db_->save();
}

/* Aggregations */

int64_t Collection::min_int(const std::string& field)
{
    int64_t result = 0;
    bool found = false;
    for (auto& doc : docs_) {
        if (doc->has(field)) {
            int64_t v = doc->get_int(field);
            if (!found || v < result) { result = v; found = true; }
        }
    }
    return result;
}

int64_t Collection::max_int(const std::string& field)
{
    int64_t result = 0;
    bool found = false;
    for (auto& doc : docs_) {
        if (doc->has(field)) {
            int64_t v = doc->get_int(field);
            if (!found || v > result) { result = v; found = true; }
        }
    }
    return result;
}

double Collection::avg(const std::string& field)
{
    double sum = 0.0;
    int n = 0;
    for (auto& doc : docs_) {
        if (doc->has(field)) {
            auto type = doc->get_type(field);
            if (type == ValType::Int || type == ValType::Double) {
                sum += doc->get_dbl(field);
                n++;
            }
        }
    }
    return n > 0 ? sum / n : 0.0;
}

int64_t Collection::sum_int(const std::string& field)
{
    int64_t sum = 0;
    for (auto& doc : docs_)
        if (doc->has(field))
            sum += doc->get_int(field);
    return sum;
}

/* ============================================================
 * Database
 * ============================================================ */

Database::~Database()
{
    if (!path_.empty())
        save();
}

std::unique_ptr<Database> Database::open(const std::string& path)
{
    return open(path, true, true);
}

std::unique_ptr<Database> Database::open(const std::string& path,
                                         bool pretty, bool auto_save)
{
    auto db = std::unique_ptr<Database>(new Database());
    db->path_ = path;
    db->pretty_ = pretty;
    db->auto_save_ = auto_save;
    db->load_from_file();
    return db;
}

std::unique_ptr<Database> Database::memory()
{
    auto db = std::unique_ptr<Database>(new Database());
    db->auto_save_ = false;
    return db;
}

Collection &Database::collection(const std::string& name)
{
    for (auto& col : collections_)
        if (col->name() == name) return *col;
    collections_.push_back(
        std::unique_ptr<Collection>(new Collection(name, this)));
    return *collections_.back();
}

void Database::drop(const std::string& name)
{
    auto it = std::find_if(collections_.begin(), collections_.end(),
        [&name](const auto& col) { return col->name() == name; });
    if (it != collections_.end()) {
        collections_.erase(it);
        if (auto_save_) save();
    }
}

void Database::save()
{
    if (path_.empty()) return;
    std::string json = detail_json_write(*this, pretty_);
    std::ofstream out(path_);
    if (out) out << json;
}

void Database::reload()
{
    if (path_.empty()) return;
    collections_.clear();
    load_from_file();
}

bool Database::export_to(const std::string& path)
{
    std::string json = detail_json_write(*this, pretty_);
    std::ofstream out(path);
    if (!out) return false;
    out << json;
    return true;
}

void Database::load_from_file()
{
    if (path_.empty()) return;
    namespace fs = std::filesystem;
    if (!fs::exists(path_) || !fs::is_regular_file(path_)) return;

    std::ifstream in(path_);
    std::string json((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
    if (!json.empty())
        detail_json_parse(*this, json);
}

} /* namespace utinydb */
