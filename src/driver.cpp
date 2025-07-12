// FILE: main.cpp

#include "ORCA.h"
#include <iostream>
#include <string>

// Include the implementations
#include "../utils/OMP-Vertex-Counter.cpp"
#include "../utils/OMP-Thread-Counter.cpp"
#include "../utils/SEQ-Counter.cpp"
#include "../src/NIORCA.cpp"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " [mode: FHM_VERTEX|UOM_VERTEX|ARR_VERTEX|FHM_THREAD|UOM_THREAD|ARR_THREAD|SEQ|NIORCA] [input_file] [output_file]" << std::endl;
        return 1;
    }

    std::string mode = argv[1];

    orca::ORCA* algorithm = nullptr;

    if (mode == "FHM_VERTEX") {
        algorithm = new ORCA_FHM_VERTEX();
    } else if (mode == "UOM_VERTEX") {
        algorithm = new ORCA_UOM_VERTEX();
    } else if (mode == "ARR_VERTEX") {
        algorithm = new ORCA_ARRAY_VERTEX();
    } else if (mode == "FHM_THREAD") {
        algorithm = new ORCA_FHM_THREAD();
    } else if (mode == "UOM_THREAD") {
        algorithm = new ORCA_UOM_THREAD();
    } else if (mode == "ARR_THREAD") {
        algorithm = new ORCA_ARRAY_THREAD();
    } else if (mode == "SEQ") {
        algorithm = new ORCA_SEQ();
    } else if (mode == "NIORCA") {
        algorithm = new NIORCA();
    } else {
        std::cerr << "Unknown mode: " << mode << std::endl;
        return 1;
    }

    std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();

    if (!algorithm->init(argc - 1, argv + 1)) {
        std::cerr << "Failed to initialize the graph." << std::endl;
        delete algorithm;
        return 1;
    }
    end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	std::cout << elapsed_seconds.count() << ","; 

    algorithm->orbit_count();

    start = std::chrono::system_clock::now();
    
    algorithm->writeResults();
    
    end = std::chrono::system_clock::now();
	elapsed_seconds = end - start;
	std::cout << elapsed_seconds.count() << std::endl; 

    delete algorithm;
    return 0;
}
