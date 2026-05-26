#include <iostream>
#include <cassert>
#include <string>
#include "../include/layer7/HTTPMessage.hpp"

using namespace std;

int main()
{
    cout << "== HTTP Message Tests ==\n\n";

    // HTTP Request serialization
    cout << "TEST: HTTP Request serialization\n";
    HTTPRequest req;
    req.method = "GET";
    req.path = "/index.html";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "10.0.0.1";
    req.headers["Connection"] = "close";
    string serialized = req.serialize();
    assert(serialized.find("GET /index.html HTTP/1.1") != string::npos);
    assert(serialized.find("Host: 10.0.0.1") != string::npos);
    assert(serialized.find("Connection: close") != string::npos);
    assert(serialized.find("\r\n\r\n") != string::npos);
    cout << "[PASS] Request serialization\n";

    // HTTP Request deserialization
    cout << "TEST: HTTP Request deserialization\n";
    string rawReq = "GET /test.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: MagiHTTP/1.0\r\n\r\nHello World";
    HTTPRequest parsed = HTTPRequest::deserialize(rawReq);
    assert(parsed.method == "GET");
    assert(parsed.path == "/test.html");
    assert(parsed.version == "HTTP/1.1");
    assert(parsed.headers.at("Host") == "example.com");
    assert(parsed.headers.at("User-Agent") == "MagiHTTP/1.0");
    assert(parsed.body == "Hello World");
    cout << "[PASS] Request deserialization\n";

    // HTTP Response make200
    cout << "TEST: HTTP Response make200\n";
    HTTPResponse resp200 = HTTPResponse::make200("text/html", "<h1>Hello</h1>");
    assert(resp200.statusCode == 200);
    assert(resp200.reasonPhrase == "OK");
    assert(resp200.headers.at("Content-Type") == "text/html");
    assert(resp200.headers.at("Connection") == "close");
    assert(resp200.body == "<h1>Hello</h1>");
    string resp200Str = resp200.serialize();
    assert(resp200Str.find("HTTP/1.1 200 OK") != string::npos);
    assert(resp200Str.find(string("Content-Length: ") + to_string(resp200.body.size())) != string::npos);
    assert(resp200Str.find("<h1>Hello</h1>") != string::npos);
    cout << "[PASS] Response make200\n";

    // HTTP Response make404
    cout << "TEST: HTTP Response make404\n";
    HTTPResponse resp404 = HTTPResponse::make404();
    assert(resp404.statusCode == 404);
    assert(resp404.reasonPhrase == "Not Found");
    assert(resp404.headers.at("Content-Type") == "text/html");
    assert(resp404.body.find("404") != string::npos);
    cout << "[PASS] Response make404\n";

    // HTTP Response make400
    cout << "TEST: HTTP Response make400\n";
    HTTPResponse resp400 = HTTPResponse::make400("Bad Request");
    assert(resp400.statusCode == 400);
    assert(resp400.reasonPhrase == "Bad Request");
    assert(resp400.body.find("400") != string::npos);
    cout << "[PASS] Response make400\n";

    // HTTP Response deserialization
    cout << "TEST: HTTP Response deserialization\n";
    string rawResp = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 5\r\n\r\nhello";
    HTTPResponse parsedResp = HTTPResponse::deserialize(rawResp);
    assert(parsedResp.statusCode == 200);
    assert(parsedResp.reasonPhrase == "OK");
    assert(parsedResp.headers.at("Content-Type") == "text/html");
    assert(parsedResp.body == "hello");
    cout << "[PASS] Response deserialization\n";

    // Round-trip request serialization
    cout << "TEST: Round-trip request\n";
    HTTPRequest req2;
    req2.method = "POST";
    req2.path = "/api/data";
    req2.version = "HTTP/1.1";
    req2.headers["Content-Type"] = "application/json";
    req2.headers["Content-Length"] = "14";
    req2.body = "{\"key\":\"val\"}";
    string s = req2.serialize();
    HTTPRequest req2b = HTTPRequest::deserialize(s);
    assert(req2b.method == "POST");
    assert(req2b.path == "/api/data");
    assert(req2b.headers.at("Content-Type") == "application/json");
    assert(req2b.body == "{\"key\":\"val\"}");
    cout << "[PASS] Round-trip request\n";

    cout << "\n===========================\n";
    cout << "HTTP Message Tests: ALL PASSED\n";
    return 0;
}