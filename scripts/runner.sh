#!/bin/bash
#SBATCH --job-name=ni-orca-benchmark
#SBATCH --partition=k2-living-labs
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --array=0-6
#SBATCH --output=output/orca_parallel_%A_%a.log
#SBATCH --error=output/orca_parallel_%A_%a.err

mkdir -p output

# Executable path
EXEC=build/niorca

# Dataset directory
DATASET_DIR=data

# Parallel modes (excluding SEQ)
MODES=("FHM_VERTEX" "UOM_VERTEX" "ARR_VERTEX" "FHM_THREAD" "UOM_THREAD" "ARR_THREAD" "NIORCA")

# Thread counts (high to low)
THREAD_COUNTS=(128 64 32 16 8 4 2 1)

# OpenMP scheduling strategies
SCHEDULERS=("static" "dynamic" "guided")

# Datasets
DATASETS=("ca-GrQc.graph" "email-Enron.graph" "facebook_combined.graph" "soc-Slashdot0811.graph" "roadNet-CA.graph" "wiki-Talk.graph" "cit-Patents.graph")

# Thread affinity configuration
export OMP_PROC_BIND=true
export OMP_PLACES=cores

# Get the dataset for this specific SLURM Array task
if [ -z "$SLURM_ARRAY_TASK_ID" ]; then
    echo "This script must be run as a SLURM array job. Use 'sbatch runner.sh'"
    # For testing outside of slurm, default to 0
    SLURM_ARRAY_TASK_ID=0
fi

dataset="${DATASETS[$SLURM_ARRAY_TASK_ID]}"
input_path="$DATASET_DIR/$dataset"
base_name=$(basename "$dataset" .graph)

OUT_FILE="output/results_${base_name}.csv"

# Output header to the separate output CSV
echo "Mode,Dataset,Threads,Schedule,Run,Read,StageI,StageII,StageIII,Write" > "$OUT_FILE"

# Run loop
for mode in "${MODES[@]}"; do
    for schedule in "${SCHEDULERS[@]}"; do
        export OMP_SCHEDULE="$schedule"

        skip_remaining_threads=false

        for threads in "${THREAD_COUNTS[@]}"; do
            if $skip_remaining_threads; then
                break
            fi

            export OMP_NUM_THREADS=$threads

            case $mode in
                "FHM_VERTEX") OPTS="--par OMP --c4 FHM --map FHM --alloc VERTEX" ;;
                "UOM_VERTEX") OPTS="--par OMP --c4 UOM --map UOM --alloc VERTEX" ;;
                "ARR_VERTEX") OPTS="--par OMP --c4 ARRAY --map ARRAY --alloc VERTEX" ;;
                "FHM_THREAD") OPTS="--par OMP --c4 FHM --map FHM --alloc THREAD" ;;
                "UOM_THREAD") OPTS="--par OMP --c4 UOM --map UOM --alloc THREAD" ;;
                "ARR_THREAD") OPTS="--par OMP --c4 ARRAY --map ARRAY --alloc THREAD" ;;
                "NIORCA")     OPTS="--par OMP --c4 FHM --map ARRAY --alloc THREAD" ;;
                *)            OPTS="" ;;
            esac

            for run in 1 2 3; do
                echo -n "$mode,$base_name,$threads,$schedule,$run," >> "$OUT_FILE"

                timeout 1200s "$EXEC" $OPTS "$input_path" /dev/null >> "$OUT_FILE"
                exit_code=$?

                if [ $exit_code -eq 124 ]; then
                    echo "Timeout" >> "$OUT_FILE"
                    skip_remaining_threads=true
                    break # break the 'run' loop to immediately drop down to next lower threads
                elif [ $exit_code -ne 0 ]; then
                    echo "Error (exit code $exit_code)" >> "$OUT_FILE"
                fi
            done
        done
    done
done

echo "✅ Parallel experiments completed for $base_name."
