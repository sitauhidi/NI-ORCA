#include "ORCA.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <omp.h>
#include <unordered_map>
using std::unordered_map;

class ORCA_FHM_THREAD : public orca::ORCA {
	public:
	void orbit_count() override {
		std::chrono::time_point<std::chrono::system_clock> start, end;
		start = std::chrono::system_clock::now();
		int *tri = (int*)calloc(m, sizeof(int));
		#pragma omp parallel for schedule(runtime)
      	for (int i = 0; i < m; i++) {
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
  
    	end = std::chrono::system_clock::now();
  		std::chrono::duration<double> elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  		start = std::chrono::system_clock::now();
  
    	std::atomic<int64_t> *C4 = new std::atomic<int64_t>[n]();
    
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
        		C4[pair.first].fetch_add(pair.second);
      		}
    	}
   
  		end = std::chrono::system_clock::now();
  		elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  		start = std::chrono::system_clock::now();
  
    	#pragma omp parallel
		{
			
			ska::flat_hash_map<int, int>* common = new ska::flat_hash_map<int, int>();
  			int *common_list = (int*)malloc(n * sizeof(int)), nc = 0;
		
			#pragma omp for schedule(runtime)
			for (int x = 0; x < n; x++) {
				orca::int64 f_12_14 = 0, f_10_13 = 0, f_13_14 = 0, f_11_13 = 0, f_7_11 = 0, f_5_8 = 0, f_6_9 = 0, f_9_12 = 0, f_4_8 = 0, f_8_12 = 0;
				orca::int64 f_14 = C4[x];
		
				for (int i = 0; i < nc; i++) (*common)[common_list[i]] = 0;
				nc = 0;
	
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
							if ((*common)[z] == 0) common_list[nc++] = z;
							(*common)[z]++;
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
							f_8_12 += ((*common)[z] - 1);
						}	
					}
				}
	
				// Solve system of equations
				orbit[x][14] = (f_14);
				orbit[x][13] = (f_13_14 - 6 * f_14) / 2;
				orbit[x][12] = (f_12_14 - 3 * f_14);
				orbit[x][11] = (f_11_13 - f_13_14 + 6 * f_14) / 2;
				orbit[x][10] = (f_10_13 - f_13_14 + 6 * f_14);
				orbit[x][9] = (f_9_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][8] = (f_8_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][7] = (f_13_14 + f_7_11 - f_11_13 - 6 * f_14) / 6;
				orbit[x][6] = (2 * f_12_14 + f_6_9 - f_9_12 - 6 * f_14) / 2;
				orbit[x][5] = (2 * f_12_14 + f_5_8 - f_8_12 - 6 * f_14);
				orbit[x][4] = (2 * f_12_14 + f_4_8 - f_8_12 - 6 * f_14);		
			}
			free(common_list);     
			delete common;
		}	
	  	end = std::chrono::system_clock::now();
  		elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  	}
};

class ORCA_UOM_THREAD : public orca::ORCA {
	public:
	void orbit_count() override {
		std::chrono::time_point<std::chrono::system_clock> start, end;
		start = std::chrono::system_clock::now();
		int *tri = (int*)calloc(m, sizeof(int));
		#pragma omp parallel for schedule(runtime)
      	for (int i = 0; i < m; i++) {
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
  
    	end = std::chrono::system_clock::now();
  		std::chrono::duration<double> elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  		start = std::chrono::system_clock::now();
  
    	std::atomic<int64_t> *C4 = new std::atomic<int64_t>[n]();
    
    	#pragma omp parallel
		{
			unordered_map<int, int64_t>* localC4 = new unordered_map<int, int64_t>();
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
        		C4[pair.first].fetch_add(pair.second);
      		}
    	}
   
  		end = std::chrono::system_clock::now();
  		elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  		start = std::chrono::system_clock::now();
  
    	#pragma omp parallel
		{
			
			unordered_map<int, int>* common = new unordered_map<int, int>();
  			int *common_list = (int*)malloc(n * sizeof(int)), nc = 0;
		
			#pragma omp for schedule(runtime)
			for (int x = 0; x < n; x++) {
				orca::int64 f_12_14 = 0, f_10_13 = 0, f_13_14 = 0, f_11_13 = 0, f_7_11 = 0, f_5_8 = 0, f_6_9 = 0, f_9_12 = 0, f_4_8 = 0, f_8_12 = 0;
				orca::int64 f_14 = C4[x];
		
				for (int i = 0; i < nc; i++) (*common)[common_list[i]] = 0;
				nc = 0;
	
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
							if ((*common)[z] == 0) common_list[nc++] = z;
							(*common)[z]++;
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
							f_8_12 += ((*common)[z] - 1);
						}	
					}
				}
	
				// Solve system of equations
				orbit[x][14] = (f_14);
				orbit[x][13] = (f_13_14 - 6 * f_14) / 2;
				orbit[x][12] = (f_12_14 - 3 * f_14);
				orbit[x][11] = (f_11_13 - f_13_14 + 6 * f_14) / 2;
				orbit[x][10] = (f_10_13 - f_13_14 + 6 * f_14);
				orbit[x][9] = (f_9_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][8] = (f_8_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][7] = (f_13_14 + f_7_11 - f_11_13 - 6 * f_14) / 6;
				orbit[x][6] = (2 * f_12_14 + f_6_9 - f_9_12 - 6 * f_14) / 2;
				orbit[x][5] = (2 * f_12_14 + f_5_8 - f_8_12 - 6 * f_14);
				orbit[x][4] = (2 * f_12_14 + f_4_8 - f_8_12 - 6 * f_14);		
			}
			free(common_list);     
			delete common;

		}	
	  	end = std::chrono::system_clock::now();
  		elapsed_seconds = end - start;
  		std::cout << elapsed_seconds.count() << ",";
  	}
};


class ORCA_ARRAY_THREAD : public orca::ORCA {
	public:
	void orbit_count() override {
    	std::chrono::time_point<std::chrono::system_clock> start, end;
  		start = std::chrono::system_clock::now();
	  	int *tri = (int*)calloc(m, sizeof(int));
	  	#pragma omp parallel for schedule(runtime)
  		for (int i = 0; i < m; i++) {
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
  
		end = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - start;
		std::cout << elapsed_seconds.count() << ",";
		start = std::chrono::system_clock::now();
	
		std::atomic<int64_t> *C4 = new std::atomic<int64_t>[n]();
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
  		            	        C4[x].fetch_add(1);
  		                	    C4[y].fetch_add(1);
  		                	    C4[z].fetch_add(1);
  		                	    C4[zz].fetch_add(1);
  		                	}
  		            	}
  		        	}
  		    	}
  			}
  			free(neigh);
  		}
  	
		end = std::chrono::system_clock::now();
		elapsed_seconds = end - start;
		std::cout << elapsed_seconds.count() << ",";
		start = std::chrono::system_clock::now();
		
		#pragma omp parallel
		{
			int *common = (int*)calloc(n, sizeof(int));
			int *common_list = (int*)malloc(n * sizeof(int)), nc = 0;

			#pragma omp for schedule(runtime)
			for (int x = 0; x < n; x++) {		
				orca::int64 f_12_14 = 0, f_10_13 = 0, f_13_14 = 0, f_11_13 = 0, f_7_11 = 0, f_5_8 = 0, f_6_9 = 0, f_9_12 = 0, f_4_8 = 0, f_8_12 = 0;
				orca::int64 f_14 = C4[x];

				for (int i = 0; i < nc; i++) common[common_list[i]] = 0;
				nc = 0;

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
							if (common[z] == 0) common_list[nc++] = z;
							common[z]++;
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
							f_8_12 += (common[z] - 1);
						}
					}
				}

				// Solve system of equations
				orbit[x][14] = (f_14);
				orbit[x][13] = (f_13_14 - 6 * f_14) / 2;
				orbit[x][12] = (f_12_14 - 3 * f_14);
				orbit[x][11] = (f_11_13 - f_13_14 + 6 * f_14) / 2;
				orbit[x][10] = (f_10_13 - f_13_14 + 6 * f_14);
				orbit[x][9] = (f_9_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][8] = (f_8_12 - 2 * f_12_14 + 6 * f_14) / 2;
				orbit[x][7] = (f_13_14 + f_7_11 - f_11_13 - 6 * f_14) / 6;
				orbit[x][6] = (2 * f_12_14 + f_6_9 - f_9_12 - 6 * f_14) / 2;
				orbit[x][5] = (2 * f_12_14 + f_5_8 - f_8_12 - 6 * f_14);
				orbit[x][4] = (2 * f_12_14 + f_4_8 - f_8_12 - 6 * f_14);		
			}
			free(common);
			free(common_list);     

		}
		end = std::chrono::system_clock::now();
		elapsed_seconds = end - start;
		std::cout << elapsed_seconds.count() << ",";
  	}
};