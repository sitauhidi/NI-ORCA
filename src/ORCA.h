#pragma once
#ifndef ORCA_H
#define ORCA_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <omp.h>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "flat_hash_map.hpp"

// Macros for accessing cached values from common2 and common3 maps
#define common2_get(x) ((common2_it = common2.find(x)) != common2.end() ? common2_it->second : 0)
#define common3_get(x) ((common3_it = common3.find(x)) != common3.end() ? common3_it->second : 0)

namespace orca {

using int64 = long long;
using PII = std::pair<int, int>;
using std::unordered_map;
using std::fstream;
using std::function;
using std::string;

struct TIII {
    int first, second, third;
};

class ORCA {
public:
    ORCA();
    virtual ~ORCA();

    int init(int argc, char* argv[]);
    virtual void orbit_count() = 0;
    void writeResults();

protected:
    // Edge pair with consistent ordering (a < b)
    struct PAIR {
        int a, b;
        PAIR(int a0, int b0);
        bool operator<(const PAIR& other) const;
        bool operator==(const PAIR& other) const;
    };

    // Hash function for PAIR
    struct hash_PAIR {
        size_t operator()(const PAIR& x) const;
    };

    // Triplet of nodes, ordered
    struct TRIPLE {
        int a, b, c;
        TRIPLE(int a0, int b0, int c0);
        bool operator<(const TRIPLE& other) const;
        bool operator==(const TRIPLE& other) const;
    };

    // Hash function for TRIPLE
    struct hash_TRIPLE {
        size_t operator()(const TRIPLE& x) const;
    };

    // Caches for common subgraphs
    unordered_map<PAIR, int, hash_PAIR> common2;
    unordered_map<TRIPLE, int, hash_TRIPLE> common3;
    unordered_map<PAIR, int, hash_PAIR>::iterator common2_it;
    unordered_map<TRIPLE, int, hash_TRIPLE>::iterator common3_it;

    // Graph properties
    int n = 0;                        // Number of nodes
    int m = 0;                        // Number of edges
    int d_max = 0;                    // Max degree
    int* deg = nullptr;              // Degree of each node
    PAIR* edges = nullptr;           // Edge list

    // Graph structure
    int** adj = nullptr;             // Adjacency list: adj[x][i] = neighbor
    PII** inc = nullptr;             // Incidence list: inc[x][i] = (neighbor, edge id)
    int* adj_matrix = nullptr;       // Bit-packed adjacency matrix
    const int adj_chunk = sizeof(int) * 8;
    function<bool(int, int)> adjacent; // Adjacency check function

    // Orbit results
    int64** orbit = nullptr;         // Orbit count per node

    // File streams
    fstream fin, fout;

    // Utility
    bool adjacent_list(int x, int y);
    bool adjacent_matrix(int x, int y);
    int getEdgeId(int x, int y);
};

} // namespace orca

#endif // ORCA_H
