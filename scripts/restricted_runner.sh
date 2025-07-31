#!/bin/bash

# Executable path
EXEC=../build/niorca

# Dataset directory
DATASET_DIR=~/SNAP

# Parallel modes (excluding SEQ)
MODES=("FHM_VERTEX" "UOM_VERTEX" "ARR_VERTEX" "FHM_THREAD" "UOM_THREAD" "ARR_THREAD" "NIORCA")

# Thread counts (high to low)
THREAD_COUNTS=(128 64 32 16 8 4 2 1)

# OpenMP scheduling strategies
SCHEDULERS=("static" "dynamic" "guided")

# Datasets
DATASETS=("soc-LiveJournal1.txt.graph")

# Thread affinity configuration
export OMP_PROC_BIND=true
export OMP_PLACES=cores

# Output header
echo "Mode,Dataset,Threads,Schedule,Run,Read,StageI,StageII,StageIII,Write"

# Run loop
for mode in "${MODES[@]}"; do
    for dataset in "${DATASETS[@]}"; do
        input_path="$DATASET_DIR/$dataset"
        base_name=$(basename "$dataset" .graph)

        for schedule in "${SCHEDULERS[@]}"; do
            export OMP_SCHEDULE="$schedule"

            skip_remaining_threads=false

            for threads in "${THREAD_COUNTS[@]}"; do
                if $skip_remaining_threads; then
                    break
                fi

                export OMP_NUM_THREADS=$threads

                echo -n "$mode,$base_name,$threads,$schedule,1,"

                timeout 1200s "$EXEC" "$mode" "$input_path" /dev/null
                exit_code=$?

                if [ $exit_code -eq 124 ]; then
                    echo "Timeout"
                    skip_remaining_threads=true
                elif [ $exit_code -ne 0 ]; then
                    echo "Error (exit code $exit_code)"
                fi
            done
        done
    done
done

echo "✅ Parallel experiments completed (single-run, early timeout skip)."
