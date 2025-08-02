# NI-ORCA: Parallel Non-Induced Graphlet Orbit Counter

NI-ORCA is a high-performance C++17 library for counting the orbits of non-induced graphlets (up to size 4) in large undirected graphs. It provides multiple parallel implementations using OpenMP with configurable thread scheduling and data structures for benchmarking and research use.

## Features

- Supports multiple execution modes:
  - `FHM_VERTEX`, `UOM_VERTEX`, `ARR_VERTEX` – Vertex-parallel using different backends
  - `FHM_THREAD`, `UOM_THREAD`, `ARR_THREAD` – Thread-private accumulators with reduction
  - `NIORCA` – Mixed parallelism optimized for large graphs
  - `SEQ` - Sequential code
- Configurable OpenMP scheduling (`static`, `dynamic`, `guided`) at runtime
- Thread pinning and binding for NUMA-aware performance tuning
- Input format: DIMACS `.graph` format
- Output: 15-dimensional orbit counts per node

## Build Instructions

```bash
git clone https://hpdc-gitlab.eeecs.qub.ac.uk/sitauhidi/niorca.git
cd niorca
make
```

## Usage
```bash
./build/niorca [MODE] [INPUT_FILE] [OUTPUT_FILE]
```

---

### Input Graph File Format

All graphs should be in `.graph` format:

```
t N M
v 0 1 2
v 1 2 1
...
e 0 1
e 1 2
...
```

* `t N M`: number of nodes `N` and edges `M`
* `v ID Label Degree`: vertex ID, label (label is required), degree (optional)
* `e u v`: edge between vertex `u` and `v`
---

### Example:

```bash
./build/niorca FHM_VERTEX datasets/dblp.graph output.txt
```

### Set OpenMP environment variables to control parallelism:

```bash
export OMP_NUM_THREADS=8
export OMP_SCHEDULE="dynamic"
export OMP_PROC_BIND=true
export OMP_PLACES=cores
```

## Benchmarking
A sample benchmarking script is provided:

```bash
cd scripts/
./runner.sh
```

This script runs all parallel modes across multiple datasets, thread counts, and OpenMP schedulers, printing results directly to the terminal.