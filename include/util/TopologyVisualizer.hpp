#pragma once
#include "core/Simulator.hpp"
#include "core/Node.hpp"
#include "core/Link.hpp"
#include "core/Interface.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

class TopologyVisualizer {
public:
    // Cetak ASCII art ke terminal
    static void printAscii(const Simulator& sim) {
        if (sim.nodes.empty()) {
            std::cout << "[Visualizer] Topologi kosong.\n";
            return;
        }

        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════╗\n";
        std::cout << "║           MAGI SYSTEM — TOPOLOGY MAP             ║\n";
        std::cout << "╚══════════════════════════════════════════════════╝\n\n";

        std::cout << "  NODES (" << sim.nodes.size() << "):\n";
        std::cout << "  " << std::string(48, '-') << "\n";
        for (const auto& [name, node] : sim.nodes) {
            std::string type = node->getType();
            std::string icon = getIcon(type);
            std::cout << "  " << icon << " [" << type << "] " << name << "\n";
        }

        std::cout << "\n  LINKS (" << sim.links.size() << "):\n";
        std::cout << "  " << std::string(48, '-') << "\n";

        for (const auto& lnk : sim.links) {
            Interface* a = lnk->getInterfaceA();
            Interface* b = lnk->getInterfaceB();
            std::string nameA = a->getOwner()->getName();
            std::string nameB = b->getOwner()->getName();
            int delay = lnk->getDelayMs();

            std::string epA = nameA + ":" + std::to_string(a->getPortNumber());
            std::string epB = nameB + ":" + std::to_string(b->getPortNumber());
            std::string label = "[" + std::to_string(delay) + "ms]";

            std::cout << "  " << epA 
                      << " ---" << label << "--- "
                      << epB << "\n";
        }

        std::cout << "\n";

        printAdjacency(sim);
    }

    // Export ke DOT format (Graphviz)
    static bool exportDot(const Simulator& sim, const std::string& filename) {
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "[Visualizer] Gagal buka file: " << filename << "\n";
            return false;
        }

        out << "graph MagiTopology {\n";
        out << "    rankdir=LR;\n";
        out << "    node [shape=box, style=filled];\n\n";

        for (const auto& [name, node] : sim.nodes) {
            std::string type = node->getType();
            std::string color = getColor(type);
            std::string shape = getShape(type);
            out << "    \"" << name << "\" [label=\"" << name 
                << "\\n(" << type << ")\""
                << ", fillcolor=" << color
                << ", shape=" << shape << "];\n";
        }

        out << "\n";

        for (const auto& lnk : sim.links) {
            Interface* a = lnk->getInterfaceA();
            Interface* b = lnk->getInterfaceB();
            std::string nameA = a->getOwner()->getName();
            std::string nameB = b->getOwner()->getName();
            int delay = lnk->getDelayMs();

            out << "    \"" << nameA << "\" -- \"" << nameB << "\""
                << " [label=\"" << delay << "ms\""
                << ", labelA=\":" << a->getPortNumber() << "\""
                << ", labelB=\":" << b->getPortNumber() << "\"];\n";
        }

        out << "}\n";
        out.close();

        std::cout << "[Visualizer] Topologi diekspor ke: " << filename << "\n";
        std::cout << "[Visualizer] Render dengan: dot -Tpng " 
                  << filename << " -o topology.png\n";
        return true;
    }

private:
    static std::string getIcon(const std::string& type) {
        if (type == "host")   return "[H]";
        if (type == "router") return "[R]";
        if (type == "switch") return "[S]";
        return "[?]";
    }

    static std::string getColor(const std::string& type) {
        if (type == "host")   return "lightblue";
        if (type == "router") return "lightyellow";
        if (type == "switch") return "lightgreen";
        return "white";
    }

    static std::string getShape(const std::string& type) {
        if (type == "host")   return "ellipse";
        if (type == "router") return "diamond";
        if (type == "switch") return "box";
        return "box";
    }

    static void printAdjacency(const Simulator& sim) {
        std::map<std::string, std::vector<std::string>> adj;
        for (const auto& lnk : sim.links) {
            std::string a = lnk->getInterfaceA()->getOwner()->getName();
            std::string b = lnk->getInterfaceB()->getOwner()->getName();
            adj[a].push_back(b);
            adj[b].push_back(a);
        }

        if (adj.empty()) return;

        std::cout << "  ADJACENCY:\n";
        std::cout << "  " << std::string(48, '-') << "\n";
        for (const auto& [node, neighbors] : adj) {
            std::cout << "  " << node << " <-> ";
            for (size_t i = 0; i < neighbors.size(); i++) {
                std::cout << neighbors[i];
                if (i + 1 < neighbors.size()) std::cout << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
};