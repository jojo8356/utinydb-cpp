/*
 * utinydb_json.cpp — Minimal JSON parser/writer for UTinyDB
 *
 * Parses and writes the format:
 * {
 *   "collection_name": [
 *     {"_id": 1, "key": "value", "num": 42, ...},
 *     ...
 *   ],
 *   ...
 * }
 */

#include "utinydb.hpp"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <cstdio>

namespace utinydb {

/* ============================================================
 * JSON Parser
 * ============================================================ */

struct JParser {
    const char *src;
    int         pos;
    int         len;
};

static void jp_skip_ws(JParser& p)
{
    while (p.pos < p.len) {
        char c = p.src[p.pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            p.pos++;
        else
            break;
    }
}

static char jp_peek(JParser& p)
{
    jp_skip_ws(p);
    if (p.pos >= p.len) return '\0';
    return p.src[p.pos];
}

static char jp_next(JParser& p)
{
    jp_skip_ws(p);
    if (p.pos >= p.len) return '\0';
    return p.src[p.pos++];
}

static bool jp_expect(JParser& p, char c)
{
    return jp_next(p) == c;
}

/* Parse a JSON string (between quotes) */
static bool jp_parse_string(JParser& p, std::string& out)
{
    jp_skip_ws(p);
    if (p.pos >= p.len || p.src[p.pos] != '"') return false;
    p.pos++; /* skip opening " */

    out.clear();
    while (p.pos < p.len && p.src[p.pos] != '"') {
        if (p.src[p.pos] == '\\') {
            p.pos++;
            if (p.pos >= p.len) return false;
            switch (p.src[p.pos]) {
                case '"':  out += '"'; break;
                case '\\': out += '\\'; break;
                case '/':  out += '/'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;
                case 'b':  out += '\b'; break;
                case 'f':  out += '\f'; break;
                default:   out += p.src[p.pos]; break;
            }
        } else {
            out += p.src[p.pos];
        }
        p.pos++;
    }
    if (p.pos >= p.len) return false;
    p.pos++; /* skip closing " */
    return true;
}

/* Parse a JSON number */
static Value jp_parse_number(JParser& p)
{
    jp_skip_ws(p);
    int start = p.pos;
    bool is_float = false;

    if (p.pos < p.len && p.src[p.pos] == '-') p.pos++;
    while (p.pos < p.len && p.src[p.pos] >= '0' && p.src[p.pos] <= '9')
        p.pos++;
    if (p.pos < p.len && p.src[p.pos] == '.') {
        is_float = true;
        p.pos++;
        while (p.pos < p.len && p.src[p.pos] >= '0' && p.src[p.pos] <= '9')
            p.pos++;
    }
    if (p.pos < p.len && (p.src[p.pos] == 'e' || p.src[p.pos] == 'E')) {
        is_float = true;
        p.pos++;
        if (p.pos < p.len && (p.src[p.pos] == '+' || p.src[p.pos] == '-'))
            p.pos++;
        while (p.pos < p.len && p.src[p.pos] >= '0' && p.src[p.pos] <= '9')
            p.pos++;
    }

    std::string tmp(p.src + start, static_cast<size_t>(p.pos - start));
    if (is_float)
        return Value{std::strtod(tmp.c_str(), nullptr)};
    else
        return Value{static_cast<int64_t>(std::strtoll(tmp.c_str(), nullptr, 10))};
}

/* Parse a JSON value and set it on a doc */
static void jp_parse_value_into(JParser& p, Doc& doc, const std::string& key)
{
    char c = jp_peek(p);

    if (c == '"') {
        std::string s;
        if (jp_parse_string(p, s))
            doc.set_str(key, s);
    } else if (c == '-' || (c >= '0' && c <= '9')) {
        Value v = jp_parse_number(p);
        if (std::holds_alternative<int64_t>(v))
            doc.set_int(key, std::get<int64_t>(v));
        else
            doc.set_dbl(key, std::get<double>(v));
    } else if (c == 't') {
        p.pos += 4; /* true */
        doc.set_bool(key, true);
    } else if (c == 'f') {
        p.pos += 5; /* false */
        doc.set_bool(key, false);
    } else if (c == 'n') {
        p.pos += 4; /* null */
        doc.set_null(key);
    } else {
        /* Skip unknown (nested objects/arrays — not supported in flat docs) */
        int depth = 0;
        while (p.pos < p.len) {
            char ch = p.src[p.pos];
            if (ch == '{' || ch == '[') depth++;
            else if (ch == '}' || ch == ']') {
                if (depth == 0) break;
                depth--;
            } else if ((ch == ',' || ch == '}') && depth == 0) {
                break;
            }
            p.pos++;
        }
    }
}

/* Parse a JSON object into a Doc */
static std::unique_ptr<Doc> jp_parse_object(JParser& p)
{
    if (!jp_expect(p, '{')) return nullptr;
    auto doc = std::make_unique<Doc>();

    if (jp_peek(p) == '}') {
        p.pos++;
        return doc;
    }

    for (;;) {
        std::string key;
        if (!jp_parse_string(p, key)) break;
        if (!jp_expect(p, ':')) break;

        jp_parse_value_into(p, *doc, key);

        if (jp_peek(p) == ',')
            p.pos++;
        else
            break;
    }
    jp_expect(p, '}');

    /* Set _id from document field */
    doc->sync_id_from_field();

    return doc;
}

/* Parse a JSON array of objects into a collection */
static void jp_parse_array(JParser& p, Collection& col)
{
    if (!jp_expect(p, '[')) return;

    if (jp_peek(p) == ']') {
        p.pos++;
        return;
    }

    for (;;) {
        auto doc = jp_parse_object(p);
        if (!doc) break;

        /* Add directly (not via insert, to preserve _id) */
        col.add_doc_raw(std::move(doc));

        if (jp_peek(p) == ',')
            p.pos++;
        else
            break;
    }
    jp_expect(p, ']');
}

bool detail_json_parse(Database& db, const std::string& json)
{
    JParser p;
    p.src = json.c_str();
    p.pos = 0;
    p.len = static_cast<int>(json.size());

    if (!jp_expect(p, '{')) return false;

    if (jp_peek(p) == '}') {
        p.pos++;
        return true;
    }

    for (;;) {
        std::string col_name;
        if (!jp_parse_string(p, col_name)) break;
        if (!jp_expect(p, ':')) break;

        Collection& col = db.collection(col_name);
        jp_parse_array(p, col);

        if (jp_peek(p) == ',')
            p.pos++;
        else
            break;
    }
    jp_expect(p, '}');
    return true;
}

/* ============================================================
 * JSON Writer
 * ============================================================ */

static void jb_indent(std::string& buf, int level, bool pretty)
{
    if (!pretty) return;
    buf.append(static_cast<size_t>(level * 2), ' ');
}

static void jb_nl(std::string& buf, bool pretty)
{
    if (pretty) buf += '\n';
}

static void jb_write_string(std::string& buf, const std::string& s)
{
    buf += '"';
    for (char c : s) {
        switch (c) {
            case '"':  buf += "\\\""; break;
            case '\\': buf += "\\\\"; break;
            case '\n': buf += "\\n"; break;
            case '\r': buf += "\\r"; break;
            case '\t': buf += "\\t"; break;
            case '\b': buf += "\\b"; break;
            case '\f': buf += "\\f"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char esc[8];
                    std::snprintf(esc, sizeof(esc), "\\u%04x",
                                  static_cast<unsigned char>(c));
                    buf += esc;
                } else {
                    buf += c;
                }
        }
    }
    buf += '"';
}

static void jb_write_value(std::string& buf, const Value& v)
{
    if (std::holds_alternative<std::nullptr_t>(v)) {
        buf += "null";
    } else if (std::holds_alternative<bool>(v)) {
        buf += std::get<bool>(v) ? "true" : "false";
    } else if (std::holds_alternative<int64_t>(v)) {
        buf += std::to_string(std::get<int64_t>(v));
    } else if (std::holds_alternative<double>(v)) {
        char num[64];
        std::snprintf(num, sizeof(num), "%.17g", std::get<double>(v));
        buf += num;
    } else if (std::holds_alternative<std::string>(v)) {
        jb_write_string(buf, std::get<std::string>(v));
    }
}

static void jb_write_doc(std::string& buf, const Doc& doc,
                          int indent, bool pretty)
{
    buf += '{';
    jb_nl(buf, pretty);

    bool first = true;
    for (const auto& [key, val] : doc.fields()) {
        if (!first) {
            buf += ',';
            jb_nl(buf, pretty);
        }
        jb_indent(buf, indent + 1, pretty);
        jb_write_string(buf, key);
        buf += pretty ? ": " : ":";
        jb_write_value(buf, val);
        first = false;
    }

    jb_nl(buf, pretty);
    jb_indent(buf, indent, pretty);
    buf += '}';
}

std::string detail_json_write(const Database& db, bool pretty)
{
    std::string buf;
    buf.reserve(1024);

    buf += '{';
    jb_nl(buf, pretty);

    for (int c = 0; c < db.collection_count(); c++) {
        const auto& col = db.collection_at(c);
        if (c > 0) {
            buf += ',';
            jb_nl(buf, pretty);
        }
        jb_indent(buf, 1, pretty);
        jb_write_string(buf, col.name());
        buf += pretty ? ": [" : ":[";
        jb_nl(buf, pretty);

        for (int d = 0; d < col.doc_count(); d++) {
            if (d > 0) {
                buf += ',';
                jb_nl(buf, pretty);
            }
            jb_indent(buf, 2, pretty);
            jb_write_doc(buf, col.doc_at(d), 2, pretty);
        }

        jb_nl(buf, pretty);
        jb_indent(buf, 1, pretty);
        buf += ']';
    }

    jb_nl(buf, pretty);
    buf += '}';
    jb_nl(buf, pretty);

    return buf;
}

} /* namespace utinydb */
