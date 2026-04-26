/*
 * Implementation reference: 
 *https://github.com/open-source-parsers/jsoncpp
 * */ 


#include "../../include/core/JsonParser.hpp"

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <utility>
#include <cctype>
#include <utility>

bool JsonValue::is_null() const { return type == NULL_TYPE; }
bool JsonValue::is_int() const { return type == INT_TYPE; }
bool JsonValue::is_string() const { return type == STRING_TYPE; }
bool JsonValue::is_array() const { return type == ARRAY_TYPE; }
bool JsonValue::is_object() const { return type == OBJECT_TYPE; }

int64_t JsonValue::as_int() const
{
    if (type != INT_TYPE)
        throw runtime_error("Json value is not an integer");
    return int_val;
}

const string &JsonValue::as_string() const
{
    if (type != STRING_TYPE)
        throw runtime_error("Json value is not a string");
    return str_val;
}

const vector<JsonValue> &JsonValue::as_array() const
{
    if (type != ARRAY_TYPE)
        throw runtime_error("Json value is not an array");
    return arr_val;
}

const map<string, JsonValue> &JsonValue::as_object() const
{
    if (type != OBJECT_TYPE)
        throw runtime_error("Json value is not an object");
    return obj_val;
}

const JsonValue &JsonValue::operator[](const string &key) const
{
    return as_object().at(key);
}

const JsonValue &JsonValue::operator[](size_t index) const
{
    return as_array().at(index);
}

bool JsonValue::has(const string &key) const
{
    if (type != OBJECT_TYPE)
        return false;
    return obj_val.find(key) != obj_val.end();
}

JsonValue JsonValue::make_null() { JsonValue v; v.type = NULL_TYPE; return v; }
JsonValue JsonValue::make_int(int64_t v) { JsonValue j; j.type = INT_TYPE; j.int_val = v; return j; }
JsonValue JsonValue::make_string(const string &v) { JsonValue j; j.type = STRING_TYPE; j.str_val = v; return j; }
JsonValue JsonValue::make_array(vector<JsonValue> v) { JsonValue j; j.type = ARRAY_TYPE; j.arr_val = move(v); return j; }
JsonValue JsonValue::make_object(map<string, JsonValue> v) { JsonValue j; j.type = OBJECT_TYPE; j.obj_val = move(v); return j; }

char JsonParser::peek()
{
    if (pos >= src.size())
        throw runtime_error("Unexpected end of JSON");
    return src[pos];
}

char JsonParser::advance()
{
    char c = peek();
    pos++;
    return c;
}

void JsonParser::skip_whitespace()
{
    while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\n' || src[pos] == '\r' || src[pos] == '\t'))
        pos++;
}

void JsonParser::expect(char c)
{
    skip_whitespace();
    if (pos >= src.size() || peek() != c)
        throw runtime_error(string("Expected '") + c + "'");
    advance();
}

JsonValue JsonParser::parse_value()
{
    skip_whitespace();

    if (pos >= src.size())
        throw runtime_error("Unexpected end of JSON");

    char c = peek();

    if (c == '"') return parse_string();
    if (c == '{') return parse_object();
    if (c == '[') return parse_array();
    if (c == '-' || isdigit(c)) return parse_number();
    if (c == 'n') return parse_null();

    throw runtime_error(string("Unexpected character: '") + c + "'");
}

JsonValue JsonParser::parse_string()
{
    expect('"');
    string result;

    while (pos < src.size() && peek() != '"')
    {
        char c = advance();

        if (c == '\\')
        {
            char esc = advance();
            switch (esc)
            {
            case '"': result += '"'; break;
            case '\\': result += '\\'; break;
            case '/': result += '/'; break;
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            default: result += esc; break;
            }
        }
        else
        {
            result += c;
        }
    }

    expect('"');
    return JsonValue::make_string(result);
}

JsonValue JsonParser::parse_number()
{
    string num;

    if (peek() == '-')
        num += advance();

    while (pos < src.size() && isdigit(src[pos]))
        num += advance();

    return JsonValue::make_int(stoll(num));
}

JsonValue JsonParser::parse_array()
{
    expect('[');
    vector<JsonValue> items;

    skip_whitespace();

    if (pos < src.size() && peek() != ']')
    {
        items.push_back(parse_value());

        while (true)
        {
            skip_whitespace();

            if (pos >= src.size() || peek() != ',')
                break;

            advance();
            items.push_back(parse_value());
        }
    }

    skip_whitespace();
    expect(']');
    return JsonValue::make_array(move(items));
}

JsonValue JsonParser::parse_object()
{
    expect('{');
    map<string, JsonValue> obj;

    skip_whitespace();

    if (pos < src.size() && peek() != '}')
    {
        string key = parse_string().as_string();
        skip_whitespace();
        expect(':');
        JsonValue val = parse_value();
        obj[key] = move(val);

        while (true)
        {
            skip_whitespace();

            if (pos >= src.size() || peek() != ',')
                break;

            advance();
            skip_whitespace();
            string k = parse_string().as_string();
            skip_whitespace();
            expect(':');
            obj[k] = parse_value();
        }
    }

    skip_whitespace();
    expect('}');
    return JsonValue::make_object(move(obj));
}

JsonValue JsonParser::parse_null()
{
    for (char c : {'n', 'u', 'l', 'l'})
    {
        if (advance() != c)
            throw runtime_error("Invalid null literal");
    }

    return JsonValue::make_null();
}

JsonValue JsonParser::parse(const string &jsonStr)
{
    JsonParser p;
    p.src = jsonStr;
    p.pos = 0;
    JsonValue result = p.parse_value();
    p.skip_whitespace();

    if (p.pos < p.src.size())
        throw runtime_error("Unexpected trailing content");

    return result;
}

JsonValue JsonParser::parseFile(const string &filepath)
{
    ifstream file(filepath);

    if (!file.is_open())
        throw runtime_error("Cannot open file: " + filepath);

    stringstream ss;
    ss << file.rdbuf();
    return parse(ss.str());
}
