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
enum class C4Strategy { LOCAL_FHM, LOCAL_UOM, LOCAL_ARRAY };
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
        std::cout << elapsed_seconds.count() << ","; // continue row for write time

        free(tri);
        if (C4_seq) free(C4_seq);
        if (C4_omp) delete[] C4_omp;
    }

private:
    void count_triangles(int i, int* tri) {
        int x = edges[i].a, y = edges[i].b;
        for (int xi = 0, yi = 0; xi < deg[x] && yi < deg[y]; ) {
            if (adj[row_ptr[x] + xi] == adj[row_ptr[y] + yi]) { 
                tri[i]++; 
                xi++;
                yi++;
            } else if (adj[row_ptr[x] + xi] < adj[row_ptr[y] + yi]) { 
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
                int y = adj[row_ptr[x] + nx];
                if (y >= x) break;
                nn = 0;
                for (int ny = 0; ny < deg[y]; ny++) {
                    int z = adj[row_ptr[y] + ny];
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
        if constexpr (C4_STRAT == C4Strategy::LOCAL_ARRAY) {
            #pragma omp parallel
            {
                int64_t *localC4 = (int64_t*)calloc(n, sizeof(int64_t));
                int *neigh = (int*)malloc(d_max * sizeof(int));
                int nn;
                #pragma omp for schedule(runtime)
                for (int x = 0; x < n; x++) {
                    for (int nx = 0; nx < deg[x]; nx++) {
                        int y = adj[row_ptr[x] + nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[row_ptr[y] + ny];
                            if (z >= y) break;
                            if (adjacent(x, z) == 0) continue;
                            neigh[nn++] = z;
                        }
                        for (int i = 0; i < nn; i++) {
                            int z = neigh[i];
                            for (int j = i + 1; j < nn; j++) {
                                int zz = neigh[j];
                                if (adjacent(z, zz)) {
                                    localC4[x]++;
                                    localC4[y]++;
                                    localC4[z]++;
                                    localC4[zz]++;
                                }
                            }
                        }
                    }
                }
                for (int i=0; i<n; i++) {
                    if (localC4[i] > 0) C4[i].fetch_add(localC4[i], std::memory_order_relaxed);
                }
                free(localC4);
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
                        int y = adj[row_ptr[x] + nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[row_ptr[y] + ny];
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
                        int y = adj[row_ptr[x] + nx];
                        if (y >= x) break;
                        nn = 0;
                        for (int ny = 0; ny < deg[y]; ny++) {
                            int z = adj[row_ptr[y] + ny];
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

        orbit[x][0] = deg[x];
        // x - middle node
        for (int nx1 = 0; nx1 < deg[x]; nx1++) {
            int y = inc[row_ptr[x] + nx1].first, ey = inc[row_ptr[x] + nx1].second;
            for (int ny = 0; ny < deg[y]; ny++) {
                int z = inc[row_ptr[y] + ny].first, ez = inc[row_ptr[y] + ny].second;
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
                int z = inc[row_ptr[x] + nx2].first, ez = inc[row_ptr[x] + nx2].second;
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
            int y = inc[row_ptr[x] + nx1].first, ey = inc[row_ptr[x] + nx1].second;
            for (int ny = 0; ny < deg[y]; ny++) {
                int z = inc[row_ptr[y] + ny].first, ez = inc[row_ptr[y] + ny].second;
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

        // Transform induced counts to non-induced counts
        int64 I_4 = orbit[x][4];
        int64 I_5 = orbit[x][5];
        int64 I_6 = orbit[x][6];
        int64 I_7 = orbit[x][7];
        int64 I_8 = orbit[x][8];
        int64 I_9 = orbit[x][9];
        int64 I_10 = orbit[x][10];
        int64 I_11 = orbit[x][11];
        int64 I_12 = orbit[x][12];
        int64 I_13 = orbit[x][13];
        int64 I_14 = orbit[x][14];
        
        orbit[x][14] = I_14;
        orbit[x][13] = I_13 + 3 * I_14;
        orbit[x][12] = I_12 + 3 * I_14;
        orbit[x][11] = I_11 + 2 * I_13 + 3 * I_14;
        orbit[x][10] = I_10 + 2 * I_12 + 2 * I_13 + 6 * I_14;
        orbit[x][9] = I_9 + 2 * I_12 + 3 * I_14;
        orbit[x][8] = I_8 + I_12 + I_13 + 3 * I_14;
        orbit[x][7] = I_7 + I_11 + I_13 + I_14;
        orbit[x][6] = I_6 + I_9 + I_10 + 2 * I_12 + I_13 + 3 * I_14;
        orbit[x][5] = I_5 + 2 * I_8 + I_10 + 2 * I_11 + 2 * I_12 + 4 * I_13 + 6 * I_14;
        orbit[x][4] = I_4 + 2 * I_8 + 2 * I_9 + I_10 + 4 * I_12 + 2 * I_13 + 6 * I_14;

        // 3-node non-induced counts
        orbit[x][2] += orbit[x][3];
        orbit[x][1] += 2 * orbit[x][3];

        for (int i = 0; i < nc; i++) {
            if constexpr (COMMON_MAP == MapType::ARRAY) common[common_list[i]] = 0;
            else (*common)[common_list[i]] = 0;
        }
    }
};

} // namespace orca

#endif // ORCA_ALGORITHM_HPP
