#ifndef TOPOLOGY_SERIALIZER_HPP
#define TOPOLOGY_SERIALIZER_HPP

#include "core/TopologyTypes.hpp"
#include <string>

class TopologySerializer
{
public:
    static std::string serialize(const TopologyConfig &config);
};

#endif