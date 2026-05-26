// main.cpp — Magi System, Milestone 0
#include "cli.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    Simulator sim;
    CLI cli(sim);

    // Opsional: load file topologi langsung dari argumen CLI
    if (argc >= 2) {
        std::string filepath = argv[1];
        std::cout << "[..] Memuat topologi awal: " << filepath << "\n";
        sim.loadTopology(filepath);
    }

    cli.run();
    return 0;
}
