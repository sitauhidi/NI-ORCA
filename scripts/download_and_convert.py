import urllib.request
import gzip
import shutil
import os
import glob
from collections import defaultdict

# List of URLs provided
urls = [
    "https://snap.stanford.edu/data/facebook_combined.txt.gz",
    "https://snap.stanford.edu/data/soc-Slashdot0811.txt.gz",
    "https://snap.stanford.edu/data/ca-GrQc.txt.gz",
    "https://snap.stanford.edu/data/cit-Patents.txt.gz",
    "https://snap.stanford.edu/data/roadNet-CA.txt.gz",
    "https://snap.stanford.edu/data/email-Enron.txt.gz",
    "https://snap.stanford.edu/data/wiki-Talk.txt.gz"
]

def convert_to_dimacs(input_file):
    output_file = input_file.rsplit('.', 1)[0] + ".graph"
    print(f"Processing {input_file}...")

    raw_edges = set()
    original_nodes = set()
    
    # Pass 1: Parse and find all unique original IDs
    with open(input_file, 'r') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            
            parts = line.split()
            if len(parts) < 2:
                continue
            
            try:
                u, v = int(parts[0]), int(parts[1])
            except ValueError:
                continue 

            if u == v:
                continue  # Remove self-loops

            # Treat as undirected for the set to remove duplicates
            raw_edges.add((min(u, v), max(u, v)))
            original_nodes.add(u)
            original_nodes.add(v)

    if not original_nodes:
        print(f"Skipping {input_file}: No valid data found.")
        return None

    # 2. Create the Mapping: Original ID -> Contiguous ID (0 to N-1)
    sorted_nodes = sorted(list(original_nodes))
    id_map = {old_id: new_id for new_id, old_id in enumerate(sorted_nodes)}
    
    num_nodes = len(id_map)
    
    # 3. Apply mapping to edges and calculate degrees
    final_edges = set()
    degree_map = defaultdict(int)
    
    for u, v in raw_edges:
        new_u, new_v = id_map[u], id_map[v]
        edge = (min(new_u, new_v), max(new_u, new_v))
        final_edges.add(edge)
        degree_map[new_u] += 1
        degree_map[new_v] += 1

    # Pass 2: Write to DIMACS format
    with open(output_file, 'w') as f:
        f.write(f"t {num_nodes} {len(final_edges)}\n")
        for new_id in range(num_nodes):
            original_id = sorted_nodes[new_id]
            f.write(f"v {new_id} {original_id} {degree_map[new_id]}\n")
        for x, y in sorted(final_edges):
            f.write(f"e {x} {y}\n")

    print(f"Successfully created {output_file} ({num_nodes} nodes, {len(final_edges)} edges).")
    return num_nodes, len(final_edges)

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    data_dir = os.path.join(script_dir, '../data')
    os.makedirs(data_dir, exist_ok=True)
    
    stats = []

    for url in urls:
        gz_name = url.split("/")[-1]
        txt_name = gz_name.replace(".gz", "")
        
        gz_file = os.path.join(data_dir, gz_name)
        txt_file = os.path.join(data_dir, txt_name)
        graph_file = txt_file.rsplit('.', 1)[0] + ".graph"
        
        # If the target graph file already exists, skip download/convert
        if os.path.exists(graph_file):
            print(f"Graph {os.path.basename(graph_file)} already exists. Skipping processing.")
            with open(graph_file, 'r') as f:
                first_line = f.readline().strip().split()
                if len(first_line) == 3 and first_line[0] == 't':
                    stats.append((os.path.basename(graph_file), int(first_line[1]), int(first_line[2])))
            continue

        try:
            # 1. Download
            if not os.path.exists(txt_file):
                print(f"Downloading {gz_name}...")
                urllib.request.urlretrieve(url, gz_file)
                
                # 2. Extract
                print(f"Extracting to {txt_name}...")
                with gzip.open(gz_file, 'rb') as f_in:
                    with open(txt_file, 'wb') as f_out:
                        shutil.copyfileobj(f_in, f_out)
                
                # Verify and clean up gz
                if os.path.exists(txt_file) and os.path.getsize(txt_file) > 0:
                    os.remove(gz_file)
                else:
                    print(f"❌ Verification failed for {txt_file}")
                    continue
            
            # 3. Convert
            result = convert_to_dimacs(txt_file)
            if result:
                stats.append((os.path.basename(graph_file), result[0], result[1]))
            
            # 4. Clean up txt
            os.remove(txt_file)
            print(f"🗑️  Removed temporary file {txt_name}")
            
        except Exception as e:
            print(f"⚠️ Error processing {url}: {e}")

    # Aggressive cleanup of any leftover temp files not covered by the primary loop
    for f in glob.glob(os.path.join(data_dir, "*.txt")):
        os.remove(f)
    for f in glob.glob(os.path.join(data_dir, "*.gz")):
        os.remove(f)

    # Print basic stats
    print("\n--- DIMACS Graph Statistics ---")
    print(f"{'Filename':<30} | {'Nodes':<10} | {'Edges':<10}")
    print("-" * 56)
    for stat in sorted(stats, key=lambda x: x[0]):
        print(f"{stat[0]:<30} | {stat[1]:<10} | {stat[2]:<10}")
    print("-" * 56)

if __name__ == "__main__":
    main()
