import networkx as nx
import os

def write_dimacs_graph(G, filename):
    with open(filename, 'w') as f:
        # DIMACS format: t N M
        f.write(f"t {G.number_of_nodes()} {G.number_of_edges()}\n")
        # v ID Label Degree
        for node in G.nodes():
            f.write(f"v {node} {node} {G.degree(node)}\n")
        # e u v
        for u, v in G.edges():
            f.write(f"e {u} {v}\n")

def generate_ba_graphs(output_dir, num_graphs=12):
    os.makedirs(output_dir, exist_ok=True)
    # Scaled up to 5 million vertices for heavy HPC testing
    sizes = [10000, 50000, 100000, 500000, 1000000, 5000000]
    m_values = [2, 5] 
    
    count = 0
    for n in sizes:
        for m in m_values:
            if count >= num_graphs: return
            print(f"Generating Barabási-Albert graph (n={n}, m={m})...")
            # BA preferentially attaches new nodes to existing ones with higher degrees
            G = nx.barabasi_albert_graph(n, m)
            filename = os.path.join(output_dir, f"ba_{n}_{m}.graph")
            write_dimacs_graph(G, filename)
            print(f"Saved {filename}")
            count += 1

if __name__ == "__main__":
    generate_ba_graphs("../data", num_graphs=12)
