#!/bin/bash
#SBATCH --job-name=train_rl_niorca
#SBATCH --partition=k2-living-labs
#SBATCH --nodes=1
#SBATCH --mem=256G
#SBATCH --output=output/train_rl_%j.log
#SBATCH --error=output/train_rl_%j.err
#SBATCH --time=24:00:00

mkdir -p output

echo "Starting Reinforcement Learning Agent Training..."
# Uncomment the learning step in train_rl_model.py before running if you want actual training
python3 ml/train_rl_model.py
echo "Training completed."
