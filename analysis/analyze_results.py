import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

def parse_csv(file_path):
    data = []
    with open(file_path, 'r') as f:
        lines = f.readlines()
        
    if not lines:
        return pd.DataFrame()
        
    header = lines[0].strip().split(',')
    
    i = 1
    while i < len(lines):
        line1 = lines[i].strip()
        if not line1:
            i += 1
            continue
            
        if "Timeout" in line1 or "Error" in line1:
            i += 1
            continue
            
        parts = line1.split(',')
        if len(parts) >= 9:
            # We expect a second line with the Write time
            if i + 1 < len(lines):
                line2 = lines[i+1].strip()
                try:
                    write_time = float(line2)
                    parts.append(str(write_time))
                    data.append(parts)
                    i += 2
                except ValueError:
                    i += 1 # Malformed
            else:
                break
        else:
            i += 1
            
    df = pd.DataFrame(data, columns=header)
    # Convert numerical columns
    numeric_cols = ['Threads', 'Run', 'Read', 'StageI', 'StageII', 'StageIII', 'Write']
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col])
        
    return df

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, '../output')
    
    csv_files = glob.glob(os.path.join(output_dir, 'results_*.csv'))
    
    dfs = []
    for f in csv_files:
        df = parse_csv(f)
        if not df.empty:
            dfs.append(df)
        
    if not dfs:
        print("No valid CSV files found.")
        return
        
    full_df = pd.concat(dfs, ignore_index=True)
    full_df['Total_Compute'] = full_df['StageI'] + full_df['StageII'] + full_df['StageIII']
    full_df['Total_Time'] = full_df['Read'] + full_df['Total_Compute'] + full_df['Write']
    
    # Save parsed data for easy verification
    full_df.to_csv(os.path.join(script_dir, 'parsed_results_all.csv'), index=False)
    
    # Compute average across runs
    agg_df = full_df.groupby(['Mode', 'Dataset', 'Threads', 'Schedule']).mean(numeric_only=True).reset_index()
    agg_df.to_csv(os.path.join(script_dir, 'aggregated_results.csv'), index=False)
    
    # Setup for plotting
    sns.set_theme(style="whitegrid")
    
    datasets = agg_df['Dataset'].unique()
    modes = agg_df['Mode'].unique()
    schedules = agg_df['Schedule'].unique()
    
    for dataset in datasets:
        ds_df = agg_df[agg_df['Dataset'] == dataset]
        
        # 1. Performance vs Threads (Compute Time) for static schedule
        plt.figure(figsize=(10, 6))
        sns.lineplot(data=ds_df[ds_df['Schedule'] == 'static'], x='Threads', y='Total_Compute', hue='Mode', marker='o')
        plt.title(f'Strong Scaling - Compute Time vs Threads ({dataset}, static schedule)')
        plt.ylabel('Compute Time (seconds)')
        plt.xlabel('Number of Threads')
        plt.xscale('log', base=2)
        plt.yscale('log')
        plt.xticks(ds_df['Threads'].unique(), ds_df['Threads'].unique())
        plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
        plt.tight_layout()
        plt.savefig(os.path.join(script_dir, f'scaling_{dataset}.png'))
        plt.close()
        
        # 2. Stage Breakdown for highest thread count (128) across modes
        high_threads_df = ds_df[(ds_df['Threads'] == 128) & (ds_df['Schedule'] == 'dynamic')].copy()
        if not high_threads_df.empty:
            high_threads_df.set_index('Mode')[['StageI', 'StageII', 'StageIII']].plot(kind='bar', stacked=True, figsize=(10,6))
            plt.title(f'Stage Breakdown @ 128 Threads ({dataset}, dynamic schedule)')
            plt.ylabel('Time (seconds)')
            plt.xticks(rotation=45)
            plt.tight_layout()
            plt.savefig(os.path.join(script_dir, f'stage_breakdown_{dataset}_128t.png'))
            plt.close()
            
    # Calculate speedup relative to single thread
    speedup_records = []
    for dataset in datasets:
        for mode in modes:
            for schedule in schedules:
                subset = agg_df[(agg_df['Dataset'] == dataset) & (agg_df['Mode'] == mode) & (agg_df['Schedule'] == schedule)]
                if subset.empty: continue
                # find 1 thread performance
                t1 = subset[subset['Threads'] == 1]['Total_Compute']
                if not t1.empty:
                    base_time = t1.values[0]
                    for _, row in subset.iterrows():
                        speedup = base_time / row['Total_Compute']
                        speedup_records.append({
                            'Dataset': dataset,
                            'Mode': mode,
                            'Schedule': schedule,
                            'Threads': row['Threads'],
                            'Speedup': speedup,
                            'Efficiency': speedup / row['Threads'] if row['Threads'] > 0 else 0
                        })
    speedup_df = pd.DataFrame(speedup_records)
    speedup_df.to_csv(os.path.join(script_dir, 'speedup_results.csv'), index=False)

    print(f"Analysis complete. Generated {len(datasets) * 2} plots and 3 CSVs in {script_dir}")

if __name__ == "__main__":
    main()
