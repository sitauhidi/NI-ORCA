#!/bin/bash
#SBATCH --job-name=ni-orca-seq
#SBATCH --partition=k2-living-labs
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --array=0-6
#SBATCH --output=output/orca_seq_%A_%a.log
#SBATCH --error=output/orca_seq_%A_%a.err

mkdir -p output

# Executable path
EXEC=build/niorca

# Dataset directory
DATASET_DIR=data

# Only SEQ mode
MODE="SEQ"

# Datasets
DATASETS=("ca-GrQc.graph" "email-Enron.graph" "facebook_combined.graph" "soc-Slashdot0811.graph" "roadNet-CA.graph" "wiki-Talk.graph" "cit-Patents.graph")

# Get the dataset for this specific SLURM Array task
if [ -z "$SLURM_ARRAY_TASK_ID" ]; then
    echo "This script must be run as a SLURM array job. Use 'sbatch scripts/seq_runner.sh'"
    # For testing outside of slurm, default to 0
    SLURM_ARRAY_TASK_ID=0
fi

dataset="${DATASETS[$SLURM_ARRAY_TASK_ID]}"
input_path="$DATASET_DIR/$dataset"
base_name=$(basename "$dataset" .graph)

OUT_FILE="output/seq_results_${base_name}.csv"

# Output CSV header
echo "Mode,Dataset,Threads,Schedule,Run,Read,StageI,StageII,StageIII,Write" > "$OUT_FILE"

for run in 1 2 3; do
    echo -n "$MODE,$base_name,1,N/A,$run," >> "$OUT_FILE"
    
    timeout 1200s "$EXEC" --par SEQ --c4 ARRAY --map ARRAY "$input_path" /dev/null >> "$OUT_FILE"
    exit_code=$?
    
    if [ $exit_code -eq 124 ]; then
        echo "Timeout" >> "$OUT_FILE"
    elif [ $exit_code -ne 0 ]; then
        echo "Error (exit code $exit_code)" >> "$OUT_FILE"
    fi
done

echo "✅ SEQ experiments completed for $base_name."
