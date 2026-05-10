#ifndef CLI_HPP
#define CLI_HPP

#include "core/Simulator.hpp"
#

#include <string>
#include <vector>
using namespace std;

class CLI {
public:
    explicit CLI(Simulator& sim);
    void run();
    bool processCommand(const string& line);

private:
    Simulator& sim;

    void handleCreate(const vector<string>& args);
    void handleLink(const vector<string>& args);
    void handleUnlink(const vector<string>& args);
    void handleSave(const vector<string>& args);
    void handleLoad(const vector<string>& args);
    void handleTopology(const vector<string>& args);
    void handleHelp(const vector<string>& args);

    vector<string> tokenize(const string& line) const;

    static void ok  (const string& msg);
    static void err (const string& msg);
    static void info(const string& msg);
};

#endif