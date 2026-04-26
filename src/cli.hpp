#ifndef CLI_HPP
#define CLI_HPP

#include "../include/core/Node.hpp"
#include "../include/core/Interface.hpp"
#include "../include/core/Link.hpp"
#include "../include/core/TopologyTypes.hpp"
#include "../include/core/TopologyParser.hpp"

#include <string>
#include <vector>
#include <map>
#include <memory>
using namespace std;


class Simulator {
public:
    map<string, unique_ptr<Node>> nodes;  
    vector<unique_ptr<Link>> links;  
    TopologyConfig topology; 


    Node* findNode(const string& name) const;
    pair<string, int> parseEndpoint(const string& endpoint) const;

    Node* createNode(const string& type, const string& name);
    bool linkNodes(const string& ep1, const string& ep2, int delayMs = 0);
    bool unlinkNodes(const string& ep1, const string& ep2);
    bool saveTopology(const string& filepath) const;
    bool loadTopology(const string& filepath);

private:
    int macCounter = 0;
    string generateMac();
    void   buildFromConfig();
    string topologyToJson() const; 
};


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

    // Output helpers
    static void ok  (const string& msg);
    static void err (const string& msg);
    static void info(const string& msg);
};

#endif 