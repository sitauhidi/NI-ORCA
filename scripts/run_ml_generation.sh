#!/bin/bash
#SBATCH --job-name=generate_ba_graphs
#SBATCH --partition=k2-living-labs
#SBATCH --nodes=1
#SBATCH --mem=500G
#SBATCH --output=output/generate_graphs_%j.log
#SBATCH --error=output/generate_graphs_%j.err
#SBATCH --time=12:00:00

mkdir -p output
mkdir -p data

source $HOME/sharedscratch/env/bin/activate

echo "Starting Barabasi-Albert Graph Generation..."
python3 ml/generate_graphs.py
echo "Graph generation completed."
