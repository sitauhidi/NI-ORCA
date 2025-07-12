#!/bin/bash

# Executable path
EXEC=../build/niorca

# Dataset directory
DATASET_DIR=~/archive-ni-orca/datasets

# Parallel modes (excluding SEQ)
MODES=("FHM_VERTEX" "UOM_VERTEX" "ARR_VERTEX" "FHM_THREAD" "UOM_THREAD" "ARR_THREAD" "NIORCA")

# Thread counts
THREAD_COUNTS=(1 2 4 8 16 32 64 128)

# OpenMP scheduling strategies
SCHEDULERS=("static" "dynamic" "guided")

# Datasets
DATASETS=("dblp.graph" "eu2005.graph" "hprd.graph" "human.graph" "patents.graph" "wordnet.graph" "yeast.graph" "youtube.graph")

# Thread affinity configuration
export OMP_PROC_BIND=true
export OMP_PLACES=cores


echo "Mode,Dataset,Threads,Schedule,Run,Read,StageI,StageII,StageIII,Write"
# Run loop
for mode in "${MODES[@]}"; do
    for dataset in "${DATASETS[@]}"; do
        input_path="$DATASET_DIR/$dataset"
        base_name=$(basename "$dataset" .graph)

        for schedule in "${SCHEDULERS[@]}"; do
            export OMP_SCHEDULE="$schedule"

            for threads in "${THREAD_COUNTS[@]}"; do
                export OMP_NUM_THREADS=$threads

                for run in 1 2 3; do
                    echo -n "$mode,$base_name,$threads,$schedule,$run,"
                    "$EXEC" "$mode" "$input_path" /dev/null
                done
            done
        done
    done
done

echo "✅ All parallel experiments completed."