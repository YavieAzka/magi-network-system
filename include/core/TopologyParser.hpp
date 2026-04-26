#ifndef TOPOLOGY_PARSER_HPP
#define TOPOLOGY_PARSER_HPP

#include "core/TopologyTypes.hpp"
#include <string>

class TopologyParser
{
public:
    static TopologyConfig parseFile(const string &filepath);
    static TopologyConfig parseString(const string &jsonStr);
};

#endif