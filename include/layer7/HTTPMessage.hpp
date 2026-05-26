#pragma once
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <sstream>
#include <algorithm>

struct HTTPRequest
{
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string serialize() const
    {
        std::ostringstream oss;
        oss << method << " " << path << " " << version << "\r\n";
        for (const auto &[key, val] : headers)
            oss << key << ": " << val << "\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }

    static HTTPRequest deserialize(const std::string &raw)
    {
        HTTPRequest req;
        req.version = "HTTP/1.1";
        std::istringstream iss(raw);
        std::string line;

        if (!std::getline(iss, line))
            return req;

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::istringstream rl(line);
        rl >> req.method >> req.path >> req.version;

        while (std::getline(iss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                break;
            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                while (!val.empty() && val[0] == ' ')
                    val.erase(val.begin());
                req.headers[key] = val;
            }
        }

        std::string remaining;
        while (std::getline(iss, remaining))
        {
            if (!remaining.empty() && remaining.back() == '\r')
                remaining.pop_back();
            if (!req.body.empty())
                req.body += "\n";
            req.body += remaining;
        }

        return req;
    }
};

struct HTTPResponse
{
    int statusCode;
    std::string reasonPhrase;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;

    HTTPResponse() : statusCode(200), version("HTTP/1.1"), reasonPhrase("") {}

    static HTTPResponse make200(const std::string &contentType, const std::string &contentBody)
    {
        HTTPResponse resp;
        resp.statusCode = 200;
        resp.reasonPhrase = "OK";
        resp.headers["Content-Type"] = contentType;
        resp.headers["Content-Length"] = std::to_string(contentBody.size());
        resp.headers["Connection"] = "close";
        resp.body = contentBody;
        return resp;
    }

    static HTTPResponse make404()
    {
        HTTPResponse resp;
        resp.statusCode = 404;
        resp.reasonPhrase = "Not Found";
        resp.headers["Content-Type"] = "text/html";
        std::string body = "<html><body><h1>404 Not Found</h1></body></html>";
        resp.headers["Content-Length"] = std::to_string(body.size());
        resp.headers["Connection"] = "close";
        resp.body = body;
        return resp;
    }

    static HTTPResponse make400(const std::string &reason)
    {
        HTTPResponse resp;
        resp.statusCode = 400;
        resp.reasonPhrase = reason.empty() ? "Bad Request" : reason;
        std::string body = "<html><body><h1>400 " + resp.reasonPhrase + "</h1></body></html>";
        resp.headers["Content-Type"] = "text/html";
        resp.headers["Content-Length"] = std::to_string(body.size());
        resp.headers["Connection"] = "close";
        resp.body = body;
        return resp;
    }

    std::string serialize() const
    {
        std::ostringstream oss;
        oss << version << " " << statusCode << " " << reasonPhrase << "\r\n";
        for (const auto &[key, val] : headers)
            oss << key << ": " << val << "\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }

    static HTTPResponse deserialize(const std::string &raw)
    {
        HTTPResponse resp;
        resp.version = "HTTP/1.1";
        std::istringstream iss(raw);
        std::string line;

        if (!std::getline(iss, line))
            return resp;

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::istringstream sl(line);
        sl >> resp.version >> resp.statusCode;

        std::string token;
        while (sl >> token)
        {
            if (!resp.reasonPhrase.empty())
                resp.reasonPhrase += " ";
            resp.reasonPhrase += token;
        }

        while (std::getline(iss, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty())
                break;
            size_t colon = line.find(':');
            if (colon != std::string::npos)
            {
                std::string key = line.substr(0, colon);
                std::string val = line.substr(colon + 1);
                while (!val.empty() && val[0] == ' ')
                    val.erase(val.begin());
                resp.headers[key] = val;
            }
        }

        std::string remaining;
        while (std::getline(iss, remaining))
        {
            if (!remaining.empty() && remaining.back() == '\r')
                remaining.pop_back();
            if (!resp.body.empty())
                resp.body += "\n";
            resp.body += remaining;
        }

        return resp;
    }
};