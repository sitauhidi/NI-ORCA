#!/bin/bash

# Executable path
EXEC=../build/niorca

# Dataset directory
DATASET_DIR=~/SNAP

# Only SEQ mode
MODE="SEQ"

# Datasets
DATASETS=("twitter_combined.txt.graph" "web-Google.txt.graph" "wiki-topcats.txt.graph" "soc-LiveJournal1.txt.graph")
#DATASETS=("dblp.graph" "eu2005.graph" "hprd.graph" "human.graph" "patents.graph" "wordnet.graph" "yeast.graph" "youtube.graph")

# Output CSV header
echo "Mode,Dataset,Threads,Schedule,Run,Read,StageI,StageII,StageIII,Write"

# Run loop
for dataset in "${DATASETS[@]}"; do
    input_path="$DATASET_DIR/$dataset"
    base_name=$(basename "$dataset" .graph)

    for run in 1 2 3; do
        echo -n "$MODE,$base_name,1,N/A,$run,"
        "$EXEC" "$MODE" "$input_path" /dev/null
    done
done

echo "✅ SEQ experiments completed."
