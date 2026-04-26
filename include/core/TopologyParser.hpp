#ifndef TOPOLOGY_PARSER_HPP
#define TOPOLOGY_PARSER_HPP

#include "TopologyTypes.hpp"
#include <string>

class TopologyParser
{
public:
    static TopologyConfig parseFile(const string &filepath);
    static TopologyConfig parseString(const string &jsonStr);
};

#endif