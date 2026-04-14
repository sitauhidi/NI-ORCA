#ifndef ORCA_ALGORITHM_HPP
#define ORCA_ALGORITHM_HPP

#include "ORCA.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <omp.h>
#include <unordered_map>
#include "flat_hash_map.hpp"

namespace orca {

enum class Parallelism { SEQ, OMP };
enum class C4Strategy { LOCAL_FHM, LOCAL_UOM, GLOBAL_ATOMIC };
enum class MapType { FHM, UOM, ARRAY };
enum class AllocMode { THREAD, VERTEX, NONE };

template <Parallelism PAR, C4Strategy C4_STRAT, MapType COMMON_MAP, AllocMode COMMON_ALLOC>
class ORCA_Generic : public ORCA {
public:
    void orbit_count() override {
        std::chrono::time_point<std::chrono::system_clock> start, end;
        start = std::chrono::system_clock::now();

        int *tri = (int*)calloc(m, sizeof(int));
        
        if constexpr (PAR == Parallelism::OMP) {
            #pragma omp parallel for schedule(runtime)
            for (int i = 0; i < m; i++) count_triangles(i, tri);
        } else {
            for (int i = 0; i < m; i++) count_triangles(i, tri);
        }

        end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        std::cout << elapsed_seconds.count() << ",";
        start = std::chrono::system_clock::now();

        // 4-Cycle counting
        int64* C4_seq = nullptr;
        std::atomic<int64_t>* C4_omp = nullptr;

        if constexpr (PAR == Parallelism::SEQ) {
            C4_seq = (int64*)calloc(n, sizeof(int64));
            count_c4_seq(C4_seq);
        } else {
            C4_omp = new std::atomic<int64_t>[n]();
            count_c4_omp(C4_omp);
        }

        end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;
        std::cout << elapsed_seconds.count() << ",";
        start = std::chrono::system_clock::now();

        // Orbit counting
        if constexpr (PAR == Parallelism::OMP) {
            if constexpr (COMMON_ALLOC == AllocMode::THREAD) {
                #pragma omp parallel
                {
                    auto common_ptr = create_common_map();
                    int *common_list = (int*)malloc(n * sizeof(int));
                    #pragma omp for schedule(runtime)
                    for (int x = 0; x < n; x++) {
                        int64 f_14 = C4_omp[x].load(std::memory_order_relaxed);
                        count_orbits_for_x(x, tri, common_ptr, common_list, f_14);
                    }
                    free_common_map(common_ptr);
                    free(common_list);
                }
            } else {
                #pragma omp parallel for schedule(runtime)
                for (int x = 0; x < n; x++) {
                    auto common_ptr = create_common_map();
                    int *common_list = (int*)malloc(n * sizeof(int));
                    int64 f_14 = C4_omp[x].load(std::memory_order_relaxed);
                    count_orbits_for_x(x, tri, common_ptr, common_list, f_14);
                    free_common_map(common_ptr);
                    free(common_list);
                }
            }
        } else {
            // Sequence mode
            auto common_ptr = create_common_map();
            int *common_list = (int*)malloc(n * sizeof(int));
            for (int x = 0; x < n; x++) {
                int64 f_14 = C4_seq[x];
                count_orbits_for_x(x, tri, common_ptr, common_list, f_14);
            }
            free_common_map(common_ptr);
            free(common_list);
        }

        end = std::chrono::system_clock::now();
        elapsed_seconds = end - start;
        std::cout << elapsed_seconds.count() << "\n"; // end of row, replace trailing comma

        free(tri);
        if (C4_seq) free(C4_seq);
        if (C4_omp) delete[] C4_omp;
    }

private:
    void count_triangles(int i, int* tri) {
        int x = edges[i].a, y = edges[i].b;
        for (int xi = 0, yi = 0; xi < deg[x] && yi < deg[y]; ) {
            if (adj[x][xi] == adj[y][yi]) { 
                tri[i]++; 
                xi++;
                yi++;
            } else if (adj[x][xi] < adj[y][yi]) { 
                xi++; 
            } else { 
                yi++; 
            }
        }
    }

    void count_c4_seq(int64* C4) {
        int *neigh = (int*)malloc(d_max * sizeof(int));
        int nn;
        for (int x = 0; x < n; x++) {
            for (int nx = 0; nx < deg[x]; nx++) {
                int y = adj[x][nx];
                if (y >= x) break;
                nn = 0;
                for (int ny = 0; ny < deg[y]; ny++) {
                    int z = adj[y][ny];
                    if (z >= y) break;
                    if (adjacent(x, z) == 0) continue;
                    neigh[nn++] = z;
                }
                for (int i = 0; i < nn; i++) {
                    int z = neigh[i];
                    for (int j = i + 1; j < nn; j++) {
                        int zz = neigh[j];
                        if (adjacent(z, zz)) {
                            C4[x]++; C4[y]++; C4[z]++; C4[zz]++;
                        }
                    }
                }
            }
        }
        free(neigh);
    }

    void count_c4_omp(std::atomic<int64_t>* C4) {
        if constexpr (C4_STRAT == C4Strategy::GLOBAL_ATOMIC) {
            #pragma omp parallel
            {
                int *neigh = (int*)malloc(n * sizeof(int));
                int nn;
                #pragma omp for schedule(runtime)
                for (int x = 0; x < n; x++) {
                    for (int nx = 0; nx < deg[x]; nx++) {
                        int y = adj[x][nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[y][ny];
                            if (z >= y) break;
                            if (adjacent(x, z) == 0) continue;
                            neigh[nn++] = z;
                        }
                        for (int i = 0; i < nn; i++) {
                            int z = neigh[i];
                            for (int j = i + 1; j < nn; j++) {
                                int zz = neigh[j];
                                if (adjacent(z, zz)) {
                                    C4[x].fetch_add(1, std::memory_order_relaxed);
                                    C4[y].fetch_add(1, std::memory_order_relaxed);
                                    C4[z].fetch_add(1, std::memory_order_relaxed);
                                    C4[zz].fetch_add(1, std::memory_order_relaxed);
                                }
                            }
                        }
                    }
                }
                free(neigh);
            }
        } else if constexpr (C4_STRAT == C4Strategy::LOCAL_FHM) {
            #pragma omp parallel
            {
                ska::flat_hash_map<int, int64_t>* localC4 = new ska::flat_hash_map<int, int64_t>();
                int *neigh = (int*)malloc(d_max * sizeof(int));
                int nn;
                #pragma omp for schedule(runtime)
                for (int x = 0; x < n; x++) {
                    for (int nx = 0; nx < deg[x]; nx++) {
                        int y = adj[x][nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[y][ny];
                            if (z >= y) break;
                            if (adjacent(x, z) == 0) continue;
                            neigh[nn++] = z;
                        }
                        for (int i = 0; i < nn; i++) {
                            int z = neigh[i];
                            for (int j = i + 1; j < nn; j++) {
                                int zz = neigh[j];
                                if (adjacent(z, zz)) {
                                    (*localC4)[x]++;
                                    (*localC4)[y]++;
                                    (*localC4)[z]++;
                                    (*localC4)[zz]++;
                                }
                            }
                        }
                    }
                }
                for (const auto& pair : *localC4) {
                    C4[pair.first].fetch_add(pair.second, std::memory_order_relaxed);
                }
                delete localC4;
                free(neigh);
            }
        } else if constexpr (C4_STRAT == C4Strategy::LOCAL_UOM) {
            #pragma omp parallel
            {
                std::unordered_map<int, int64_t>* localC4 = new std::unordered_map<int, int64_t>();
                int *neigh = (int*)malloc(d_max * sizeof(int));
                int nn;
                #pragma omp for schedule(runtime)
                for (int x = 0; x < n; x++) {
                    for (int nx = 0; nx < deg[x]; nx++) {
                        int y = adj[x][nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[y][ny];
                            if (z >= y) break;
                            if (adjacent(x, z) == 0) continue;
                            neigh[nn++] = z;
                        }
                        for (int i = 0; i < nn; i++) {
                            int z = neigh[i];
                            for (int j = i + 1; j < nn; j++) {
                                int zz = neigh[j];
                                if (adjacent(z, zz)) {
                                    (*localC4)[x]++;
                                    (*localC4)[y]++;
                                    (*localC4)[z]++;
                                    (*localC4)[zz]++;
                                }
                            }
                        }
                    }
                }
                for (const auto& pair : *localC4) {
                    C4[pair.first].fetch_add(pair.second, std::memory_order_relaxed);
                }
                delete localC4;
                free(neigh);
            }
        }
    }

    auto create_common_map() {
        if constexpr (COMMON_MAP == MapType::FHM) return new ska::flat_hash_map<int, int>();
        else if constexpr (COMMON_MAP == MapType::UOM) return new std::unordered_map<int, int>();
        else if constexpr (COMMON_MAP == MapType::ARRAY) return (int*)calloc(n, sizeof(int));
    }

    template <typename T>
    void free_common_map(T ptr) {
        if constexpr (COMMON_MAP == MapType::ARRAY) free(ptr);
        else delete ptr;
    }

    template <typename CommonPtr>
    inline void count_orbits_for_x(int x, int* tri, CommonPtr common, int* common_list, int64 f_14) {
        int nc = 0;
        int64 f_12_14 = 0, f_10_13 = 0, f_13_14 = 0, f_11_13 = 0, f_7_11 = 0, f_5_8 = 0, f_6_9 = 0, f_9_12 = 0, f_4_8 = 0, f_8_12 = 0;

        for (int i = 0; i < nc; i++) {
            if constexpr (COMMON_MAP == MapType::ARRAY) common[common_list[i]] = 0;
            else (*common)[common_list[i]] = 0;
        }

        orbit[x][0] = deg[x];
        // x - middle node
        for (int nx1 = 0; nx1 < deg[x]; nx1++) {
            int y = inc[x][nx1].first, ey = inc[x][nx1].second;
            for (int ny = 0; ny < deg[y]; ny++) {
                int z = inc[y][ny].first, ez = inc[y][ny].second;
                if (adjacent(x, z)) { // triangle
                    if (z < y) {
                        f_12_14 += tri[ez] - 1;
                        f_10_13 += (deg[y] - 1 - tri[ez]) + (deg[z] - 1 - tri[ez]);
                    }
                } else {
                    if constexpr (COMMON_MAP == MapType::ARRAY) {
                        if (common[z] == 0) common_list[nc++] = z;
                        common[z]++;
                    } else {
                        if ((*common)[z] == 0) common_list[nc++] = z;
                        (*common)[z]++;
                    }
                }
            }
            for (int nx2 = nx1 + 1; nx2 < deg[x]; nx2++) {
                int z = inc[x][nx2].first, ez = inc[x][nx2].second;
                if (adjacent(y, z)) { // triangle
                    orbit[x][3]++;
                    f_13_14 += (tri[ey] - 1) + (tri[ez] - 1);
                    f_11_13 += (deg[x] - 1 - tri[ey]) + (deg[x] - 1 - tri[ez]);
                } else { // path
                    orbit[x][2]++;
                    f_7_11 += (deg[x] - 1 - tri[ey] - 1) + (deg[x] - 1 - tri[ez] - 1);
                    f_5_8 += (deg[y] - 1 - tri[ey]) + (deg[z] - 1 - tri[ez]);
                }
            }
        }
        // x - side node
        for (int nx1 = 0; nx1 < deg[x]; nx1++) {
            int y = inc[x][nx1].first, ey = inc[x][nx1].second;
            for (int ny = 0; ny < deg[y]; ny++) {
                int z = inc[y][ny].first, ez = inc[y][ny].second;
                if (x == z) continue;
                if (!adjacent(x, z)) { // path
                    orbit[x][1]++;
                    f_6_9 += (deg[y] - 1 - tri[ey] - 1);
                    f_9_12 += tri[ez];
                    f_4_8 += (deg[z] - 1 - tri[ez]);
                    if constexpr (COMMON_MAP == MapType::ARRAY) {
                        f_8_12 += (common[z] - 1);
                    } else {
                        f_8_12 += ((*common)[z] - 1);
                    }
                }
            }
        }

        solve_equations(x, f_14, f_13_14, f_12_14, f_11_13, f_10_13, f_9_12, f_8_12, f_7_11, f_6_9, f_5_8, f_4_8);
    }
};

} // namespace orca

#endif // ORCA_ALGORITHM_HPP
