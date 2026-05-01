#include "ORCA.h"
#include "ORCA_algorithm.hpp"
#include <iostream>
#include <string>

using namespace orca;

void print_usage(const char* prog_name) {
    std::cerr << "Usage: " << prog_name << " [options] <input_file> <output_file>\n"
              << "Options:\n"
              << "  --par   <SEQ|OMP>        Parallelism mode (default: OMP)\n"
              << "  --c4    <FHM|UOM|ARRAY>  4-Cycle counting strategy (default: FHM)\n"
              << "  --map   <FHM|UOM|ARRAY>  Common neighbor map type (default: ARRAY)\n"
              << "  --alloc <THREAD|VERTEX>  Map allocation strategy (default: THREAD)\n";
}

template<Parallelism PAR, C4Strategy C4_STRAT, MapType COMMON_MAP>
ORCA* create_alloc(const std::string& alloc) {
    if (alloc == "THREAD") return new ORCA_Generic<PAR, C4_STRAT, COMMON_MAP, AllocMode::THREAD>();
    if (alloc == "VERTEX") return new ORCA_Generic<PAR, C4_STRAT, COMMON_MAP, AllocMode::VERTEX>();
    return nullptr;
}

template<Parallelism PAR, C4Strategy C4_STRAT>
ORCA* create_map(const std::string& map_t, const std::string& alloc) {
    if (map_t == "FHM") return create_alloc<PAR, C4_STRAT, MapType::FHM>(alloc);
    if (map_t == "UOM") return create_alloc<PAR, C4_STRAT, MapType::UOM>(alloc);
    if (map_t == "ARRAY") return create_alloc<PAR, C4_STRAT, MapType::ARRAY>(alloc);
    return nullptr;
}

template<Parallelism PAR>
ORCA* create_c4(const std::string& c4, const std::string& map_t, const std::string& alloc) {
    if (c4 == "FHM") return create_map<PAR, C4Strategy::LOCAL_FHM>(map_t, alloc);
    if (c4 == "UOM") return create_map<PAR, C4Strategy::LOCAL_UOM>(map_t, alloc);
    if (c4 == "ARRAY") return create_map<PAR, C4Strategy::LOCAL_ARRAY>(map_t, alloc);
    return nullptr;
}

ORCA* build_algorithm(const std::string& par, const std::string& c4, const std::string& map_t, const std::string& alloc) {
    if (par == "SEQ") {
        // For SEQ, alloc mode is NONE conceptually, we just map it down efficiently
        if (c4 == "FHM") return create_map<Parallelism::SEQ, C4Strategy::LOCAL_FHM>(map_t, "THREAD");
        if (c4 == "UOM") return create_map<Parallelism::SEQ, C4Strategy::LOCAL_UOM>(map_t, "THREAD");
        if (c4 == "ARRAY") return create_map<Parallelism::SEQ, C4Strategy::LOCAL_ARRAY>(map_t, "THREAD");
        return nullptr;
    }
    if (par == "OMP") return create_c4<Parallelism::OMP>(c4, map_t, alloc);
    return nullptr;
}

int main(int argc, char* argv[]) {
    std::string par = "OMP";
    std::string c4 = "FHM";
    std::string map_t = "ARRAY";
    std::string alloc = "THREAD";
    std::string input_file = "";
    std::string output_file = "";
    bool features_only = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--par" && i + 1 < argc) par = argv[++i];
        else if (arg == "--c4" && i + 1 < argc) c4 = argv[++i];
        else if (arg == "--map" && i + 1 < argc) map_t = argv[++i];
        else if (arg == "--alloc" && i + 1 < argc) alloc = argv[++i];
        else if (arg == "--features-only") features_only = true;
        else if (arg == "-h" || arg == "--help") { print_usage(argv[0]); return 0; }
        else if (input_file.empty()) input_file = arg;
        else if (output_file.empty()) output_file = arg;
        else {
            std::cerr << "Unknown argument: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_file.empty() || output_file.empty()) {
        std::cerr << "Error: missing input or output file.\n";
        print_usage(argv[0]);
        return 1;
    }

    ORCA* algorithm = build_algorithm(par, c4, map_t, alloc);
    if (!algorithm) {
        std::cerr << "Error: Invalid combination of algorithm parameters.\n";
        return 1;
    }

    std::chrono::time_point<std::chrono::system_clock> start, end;
    start = std::chrono::system_clock::now();

    // Reconstruct fake argv for ORCA init() if it expects [prog, input, output]
    char* fake_argv[] = { argv[0], const_cast<char*>(input_file.c_str()), const_cast<char*>(output_file.c_str()) };
    if (!algorithm->init(3, fake_argv)) {
        std::cerr << "Failed to initialize the graph.\n";
        delete algorithm;
        return 1;
    }
    
    if (features_only) {
        algorithm->extract_and_print_features();
        delete algorithm;
        return 0;
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
