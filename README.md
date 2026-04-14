# NI-ORCA: Parallel Non-Induced Graphlet Orbit Counter

NI-ORCA is a high-performance C++17 library for computing the orbit counts of non-induced graphlets (up to size 4) in large undirected graphs. It features an orthogonal, highly configurable template-based architecture to allow HPC researchers to benchmark various parallelization techniques independently.

## Features
- **Configurable Parallel Strategies:** Instead of hardcoded binaries, compose the perfect OpenMP parallel loop:
  - `--par <SEQ|OMP>`: Run sequentially or multi-threaded.
  - `--c4 <FHM|UOM|ARRAY>`: The data structure strategy for the 4-cycle loop.
  - `--map <FHM|UOM|ARRAY>`: The backend data structure for thread/node local mappings.
  - `--alloc <THREAD|VERTEX>`: Allocate memory maps purely per thread, or freshly per vertex.
- **SLURM Ready distributed execution:** Test scaling directly utilizing clusters and arrays.
- **Automated Dataset Conversion:** Fetch, extract, and convert SNAP sets into the `DIMACS` `.graph` standard via Python.

## Build Instructions

```bash
git clone https://hpdc-gitlab.eeecs.qub.ac.uk/sitauhidi/niorca.git
cd niorca
make -j4
```

## Data Preparation

To download the benchmark datasets to the `data/` directory and convert them to the expected DIMACS `.graph` format, run:

```bash
python3 scripts/download_and_convert.py
```
This automatically verifies the files and cleans up temporary compressed archives.

## Usage

```bash
./build/niorca [options] <input_file> <output_file>

Options:
  --par   <SEQ|OMP>        Parallelism mode (default: OMP)
  --c4    <FHM|UOM|ARRAY>  4-Cycle counting strategy (default: FHM)
  --map   <FHM|UOM|ARRAY>  Common neighbor map type (default: ARRAY)
  --alloc <THREAD|VERTEX>  Map allocation strategy (default: THREAD)
```

### Example

```bash
./build/niorca --par OMP --c4 FHM --map ARRAY --alloc THREAD data/ca-GrQc.graph output.txt
```

---

### Input Graph File Format
All graphs should be formatted via our `download_and_convert.py` tool. The expected `.graph` DIMACS structure looks like:

```
t N M
v 0 1 2
v 1 2 1
...
e 0 1
e 1 2
...
```

* `t N M`: total nodes `N` and edges `M`
* `v ID Label Degree`: contiguous vertex ID, original label reference, degree.
* `e u v`: an undirected edge between vertex `u` and `v`

## HPC Benchmarking

To collect massive arrays of data testing how well algorithms scale per thread/thread-type/scheduler, SLURM scripts are natively provided. 
From the **root directory**, simply execute:

```bash
# To test scaling parallel strategies across all dimensions
sbatch scripts/runner.sh
```

- Tests automatically utilize `SLURM Arrays` (`--array=0-6`) to parallelize testing instances, allocating 1 dataset exactly to 1 cluster node.
- Standard cluster logs and algorithmic scaling `.csv` sheets dump neatly into the `output/` directory dynamically.
- `scripts/seq_runner.sh` serves to run unthreaded sequential modes strictly to calculate your multi-threaded speedups.