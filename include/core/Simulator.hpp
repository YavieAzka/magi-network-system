#ifndef SIMULATOR_HPP
#define SIMULATOR_HPP

#include "core/Node.hpp"
#include "core/Interface.hpp"
#include "core/Link.hpp"
#include "core/TopologyTypes.hpp"
#include "util/TopologyParser.hpp"
#include "util/TopologySerializer.hpp"

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
    void buildFromConfig();
};

#endif