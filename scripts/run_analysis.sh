#!/bin/bash
#SBATCH --job-name=ni-orca-analysis
#SBATCH --partition=k2-living-labs
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --output=analysis/analysis_%j.log
#SBATCH --error=analysis/analysis_%j.err

# Source the virtual environment
if [ -f "$HOME/sharedscratch/env/bin/activate" ]; then
    echo "Sourcing virtual environment..."
    source $HOME/sharedscratch/env/bin/activate
else
    echo "Warning: Virtual environment not found at $HOME/sharedscratch/env/bin/activate"
fi

# Ensure required packages are installed
pip install --quiet pandas matplotlib seaborn

# Run the analysis
echo "Running analysis script..."
python3 analysis/analyze_results.py

echo "Analysis Job Completed."
