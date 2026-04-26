#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdint>
using namespace std;

struct JsonValue
{
    enum Type
    {
        NULL_TYPE,
        INT_TYPE,
        STRING_TYPE,
        ARRAY_TYPE,
        OBJECT_TYPE
    };

    Type type = NULL_TYPE;
    int64_t int_val = 0;
    string str_val;
    vector<JsonValue> arr_val;
    map<string, JsonValue> obj_val;

    bool is_null() const;
    bool is_int() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    int64_t as_int() const;
    const string &as_string() const;
    const vector<JsonValue> &as_array() const;
    const map<string, JsonValue> &as_object() const;

    const JsonValue &operator[](const string &key) const;
    const JsonValue &operator[](size_t index) const;

    bool has(const string &key) const;

    static JsonValue make_null();
    static JsonValue make_int(int64_t v);
    static JsonValue make_string(const string &v);
    static JsonValue make_array(vector<JsonValue> v);
    static JsonValue make_object(map<string, JsonValue> v);
};

class JsonParser
{
private:
    string src;
    size_t pos;

    char peek();
    char advance();
    void skip_whitespace();
    void expect(char c);

    JsonValue parse_value();
    JsonValue parse_string();
    JsonValue parse_number();
    JsonValue parse_array();
    JsonValue parse_object();
    JsonValue parse_null();

public:
    static JsonValue parse(const string &jsonStr);
    static JsonValue parseFile(const string &filepath);
};

#endif