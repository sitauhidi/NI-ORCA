#include "ORCA.h"
#include <set>
#include <iostream>
#include <algorithm>

using namespace orca;

ORCA::PAIR::PAIR(int a0, int b0) {
    a = std::min(a0, b0);
    b = std::max(a0, b0);
}

bool ORCA::PAIR::operator<(const PAIR &other) const {
    if (a == other.a) return b < other.b;
    return a < other.a;
}

bool ORCA::PAIR::operator==(const PAIR &other) const {
    return a == other.a && b == other.b;
}

size_t ORCA::hash_PAIR::operator()(const PAIR &x) const {
    return (x.a << 8) ^ (x.b << 0);
}

// Graph::TRIPLE methods
ORCA::TRIPLE::TRIPLE(int a0, int b0, int c0) {
    a = a0; b = b0; c = c0;
    if (a > b) std::swap(a, b);
    if (b > c) std::swap(b, c);
    if (a > b) std::swap(a, b);
}

bool ORCA::TRIPLE::operator<(const TRIPLE &other) const {
    if (a == other.a) {
        if (b == other.b) return c < other.c;
        return b < other.b;
    }
    return a < other.a;
}

bool ORCA::TRIPLE::operator==(const TRIPLE &other) const {
    return a == other.a && b == other.b && c == other.c;
}

size_t ORCA::hash_TRIPLE::operator()(const TRIPLE &x) const {
    return (x.a << 16) ^ (x.b << 8) ^ (x.c << 0);
}

// Graph class constructor and destructor
ORCA::ORCA() : n(0), m(0), d_max(0), deg(nullptr), edges(nullptr),
                 row_ptr(nullptr), adj(nullptr), inc(nullptr), new_id(nullptr), adj_matrix(nullptr), orbit(nullptr) {}

ORCA::~ORCA() {
    if (deg) free(deg);
    if (edges) free(edges);
    if (row_ptr) free(row_ptr);
    if (adj) free(adj);
    if (inc) free(inc);
    if (new_id) free(new_id);
    if (adj_matrix) free(adj_matrix);
    if (orbit) {
        for (int i = 0; i < n; ++i) free(orbit[i]);
        free(orbit);
    }
}

bool ORCA::adjacent_list(int x, int y) {
    return std::binary_search(adj + row_ptr[x], adj + row_ptr[x] + deg[x], y);
}

bool ORCA::adjacent_matrix(int x, int y) {
    return adj_matrix[(x * n + y) / 32] & (1 << ((x * n + y) % 32));
}

int ORCA::getEdgeId(int x, int y) {
    return inc[row_ptr[x] + (std::lower_bound(adj + row_ptr[x], adj + row_ptr[x] + deg[x], y) - (adj + row_ptr[x]))].second;
}

int ORCA::init(int argc, char *argv[]) {
  	// open input, output files
  	if (argc!=3) {
  		std::cerr << "Incorrect number of arguments." << std::endl;
  		std::cerr << "Usage: " << argv[0] << " [input file] [output file]" << std::endl;
  		return 0;
  	}
  	fin.open(argv[1], fstream::in);
  	if (fin.fail()) {
  		std::cerr << "Failed to open file " << argv[2] << std::endl;
  		return 0;
  	}
  	fout.open(argv[2], fstream::out | fstream::binary);
  	if (fout.fail()) {
  		std::cerr << "Failed to open file " << argv[2] << std::endl;
  		return 0;
  	}
  	// read input graph
  	char temp;
  	fin >> temp >> n >> m;
  	if (temp != 't') exit(0);
  	int a,b,c;
  	for (int i=0;i<n;i++) {
  		fin >> temp >> a >> b >> c;
  	}
  
  	edges = (PAIR*)malloc(m*sizeof(PAIR));
  	deg = (int*)calloc(n,sizeof(int));
  	for (int i=0;i<m;i++) {
  		int a,b;
          fin >> temp >> a >> b;
          if (temp != 'e') exit(0);
     		if (!(0<=a && a<n) || !(0<=b && b<n)) {
  			std::cerr << "Node ids should be between 0 and n-1." << std::endl;
  			return 0;
  		}
  		if (a==b) {
  			std::cerr << "Self loops (edge from x to x) are not allowed." << std::endl;
  			return 0;
  		}
  		deg[a]++; deg[b]++;
  		edges[i]=PAIR(a,b);
  	}

    // relabeling
    int* order = (int*)malloc(n * sizeof(int));
    for(int i=0; i<n; i++) order[i] = i;
    std::sort(order, order+n, [&](int a, int b) { return deg[a] > deg[b]; });
    new_id = (int*)malloc(n * sizeof(int));
    for(int i=0; i<n; i++) new_id[order[i]] = i;
    
    int* new_deg = (int*)calloc(n, sizeof(int));
    for(int i=0; i<n; i++) new_deg[new_id[i]] = deg[i];
    free(deg);
    deg = new_deg;

    for (int i=0;i<m;i++) {
        edges[i].a = new_id[edges[i].a];
        edges[i].b = new_id[edges[i].b];
        if (edges[i].a > edges[i].b) std::swap(edges[i].a, edges[i].b);
    }
    free(order);

  	for (int i=0;i<n;i++) d_max=std::max(d_max,deg[i]);
  	fin.close();
  	if ((int)(std::set<PAIR>(edges,edges+m).size())!=m) {
  		std::cerr << "Input file contains duplicate undirected edges." << std::endl;
  		return 0;
  	}
  	// set up adjacency matrix if it's smaller than 100MB
  	if ((int64)n*n < 100LL*1024*1024*8) {
  		adjacent = [this](int x, int y) { return adjacent_matrix(x, y); };
  		adj_matrix = (int*)calloc((n*n)/adj_chunk+1,sizeof(int));
  		for (int i=0;i<m;i++) {
  			int a=edges[i].a, b=edges[i].b;
  			adj_matrix[(a*n+b)/adj_chunk]|=(1<<((a*n+b)%adj_chunk));
  			adj_matrix[(b*n+a)/adj_chunk]|=(1<<((b*n+a)%adj_chunk));
  		}
  	} else {
  		adjacent = [this](int x, int y) { return adjacent_list(x, y); };
  	}
  	// set up adjacency, incidence lists
    row_ptr = (int*)calloc(n + 1, sizeof(int));
    for (int i=0; i<n; i++) row_ptr[i+1] = row_ptr[i] + deg[i];
    adj = (int*)malloc(2 * m * sizeof(int));
    inc = (PII*)malloc(2 * m * sizeof(PII));

  	int *d = (int*)calloc(n,sizeof(int));
  	for (int i=0;i<m;i++) {
  		int a=edges[i].a, b=edges[i].b;
        int pos_a = row_ptr[a] + d[a];
        int pos_b = row_ptr[b] + d[b];
  		adj[pos_a]=b; adj[pos_b]=a;
  		inc[pos_a]=PII(b,i); inc[pos_b]=PII(a,i);
  		d[a]++; d[b]++;
  	}
    free(d);
  	for (int i=0;i<n;i++) {
  		std::sort(adj + row_ptr[i], adj + row_ptr[i] + deg[i]);
  		std::sort(inc + row_ptr[i], inc + row_ptr[i] + deg[i]);
  	}
  	// initialize orbit counts
  	orbit = (int64**)malloc(n*sizeof(int64*));
  	for (int i=0;i<n;i++) orbit[i] = (int64*)calloc(15,sizeof(int64));
  	return 1;
}

void ORCA::writeResults() {
    for (int i = 0; i < n; i++) {
        int mapped = new_id[i];
        for (int j = 0; j < 15; j++) {
            if (j != 0) fout << ",";
            fout << orbit[mapped][j];
        }
        fout << std::endl;
    }
}

void ORCA::extract_and_print_features() {
    if (n == 0) return;
    int max_d = 0;
    int min_d = n;
    double sum_d = 0;
    for (int i = 0; i < n; i++) {
        max_d = std::max(max_d, deg[i]);
        min_d = std::min(min_d, deg[i]);
        sum_d += deg[i];
    }
    double avg_d = sum_d / n;
    double var_d = 0;
    for (int i = 0; i < n; i++) {
        var_d += (deg[i] - avg_d) * (deg[i] - avg_d);
    }
    var_d /= n;
    double density = (double)(2LL * m) / ((double)n * (n - 1));

    std::cout << n << "," << m << "," << density << "," << min_d << "," << max_d << "," << avg_d << "," << var_d << std::endl;
}
