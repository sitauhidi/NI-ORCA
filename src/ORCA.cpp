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
                 adj(nullptr), inc(nullptr), adj_matrix(nullptr), orbit(nullptr) {}

ORCA::~ORCA() {
    if (deg) free(deg);
    if (edges) free(edges);
    if (adj) {
        for (int i = 0; i < n; ++i) free(adj[i]);
        free(adj);
    }
    if (inc) {
        for (int i = 0; i < n; ++i) free(inc[i]);
        free(inc);
    }
    if (adj_matrix) free(adj_matrix);
    if (orbit) {
        for (int i = 0; i < n; ++i) free(orbit[i]);
        free(orbit);
    }
}

bool ORCA::adjacent_list(int x, int y) {
    return std::binary_search(adj[x], adj[x] + deg[x], y);
}

bool ORCA::adjacent_matrix(int x, int y) {
    return adj_matrix[(x * n + y) / 32] & (1 << ((x * n + y) % 32));
}

int ORCA::getEdgeId(int x, int y) {
    return inc[x][std::lower_bound(adj[x], adj[x] + deg[x], y) - adj[x]].second;
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
  	adj = (int**)malloc(n*sizeof(int*));
  	for (int i=0;i<n;i++) adj[i] = (int*)malloc(deg[i]*sizeof(int));
  	inc = (PII**)malloc(n*sizeof(PII*));
  	for (int i=0;i<n;i++) inc[i] = (PII*)malloc(deg[i]*sizeof(PII));
  	int *d = (int*)calloc(n,sizeof(int));
  	for (int i=0;i<m;i++) {
  		int a=edges[i].a, b=edges[i].b;
  		adj[a][d[a]]=b; adj[b][d[b]]=a;
  		inc[a][d[a]]=PII(b,i); inc[b][d[b]]=PII(a,i);
  		d[a]++; d[b]++;
  	}
  	for (int i=0;i<n;i++) {
  		std::sort(adj[i],adj[i]+deg[i]);
  		std::sort(inc[i],inc[i]+deg[i]);
  	}
  	// initialize orbit counts
  	orbit = (int64**)malloc(n*sizeof(int64*));
  	for (int i=0;i<n;i++) orbit[i] = (int64*)calloc(15,sizeof(int64));
  	return 1;
}

void ORCA::writeResults() {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 15; j++) {
            if (j != 0) fout << ",";
            fout << orbit[i][j];
        }
        fout << std::endl;
    }
}
