import os
import subprocess
import numpy as np

try:
    import gymnasium as gym
    from gymnasium import spaces
    from stable_baselines3 import PPO
    from stable_baselines3.common.env_checker import check_env
except ImportError:
    print("Warning: Missing required ML libraries. Please run: pip install gymnasium stable-baselines3")
    gym = None

class NIORCAEnv(gym.Env if gym else object):
    """
    Custom Environment that follows gym interface for predicting the optimal
    NI-ORCA configuration based on Graph topological features.
    """
    metadata = {'render.modes': ['console']}

    def __init__(self, data_dir="../data", bin_path="../build/niorca"):
        super(NIORCAEnv, self).__init__()
        self.data_dir = data_dir
        self.bin_path = bin_path
        
        # Load all BA graphs
        if os.path.exists(data_dir):
            self.graphs = [os.path.join(data_dir, f) for f in os.listdir(data_dir) if f.endswith(".graph") and f.startswith("ba_")]
        else:
            self.graphs = []
            
        if len(self.graphs) == 0:
            print(f"Warning: No BA graphs found in {data_dir}. Run generate_graphs.py first.")
            self.graphs = ["dummy"]
            
        self.current_graph_idx = 0
        
        # Action space maps to template configurations
        # Par: [SEQ, OMP] -> 2
        # C4: [FHM, UOM, ARRAY] -> 3
        # Map: [FHM, UOM, ARRAY] -> 3
        # Alloc: [THREAD, VERTEX] -> 2
        # Schedule (passed via env var): [static, dynamic, guided] -> 3
        self.par_opts = ["SEQ", "OMP"]
        self.c4_opts = ["FHM", "UOM", "ARRAY"]
        self.map_opts = ["FHM", "UOM", "ARRAY"]
        self.alloc_opts = ["THREAD", "VERTEX"]
        self.sched_opts = ["static", "dynamic", "guided"]
        
        if gym:
            # We use MultiDiscrete to pick one option from each category
            self.action_space = spaces.MultiDiscrete([
                len(self.par_opts), 
                len(self.c4_opts), 
                len(self.map_opts), 
                len(self.alloc_opts),
                len(self.sched_opts)
            ])
            
            # State vector: [n, m, density, min_d, max_d, avg_d, var_d]
            self.observation_space = spaces.Box(low=0, high=np.inf, shape=(7,), dtype=np.float32)

    def _get_graph_features(self, filepath):
        if filepath == "dummy": return np.zeros(7, dtype=np.float32)
        cmd = [self.bin_path, "--features-only", filepath, "dummy.txt"]
        try:
            res = subprocess.run(cmd, capture_output=True, text=True)
            # Output format: n,m,density,min_d,max_d,avg_d,var_d
            features = list(map(float, res.stdout.strip().split(",")))
            return np.array(features, dtype=np.float32)
        except Exception as e:
            print(f"Feature extraction failed: {e}")
            return np.zeros(7, dtype=np.float32)

    def reset(self, seed=None, options=None):
        if gym: super().reset(seed=seed)
        # Randomly select a graph for this episode
        self.current_graph_idx = np.random.randint(0, len(self.graphs))
        graph_path = self.graphs[self.current_graph_idx]
        
        state = self._get_graph_features(graph_path)
        return state, {}

    def step(self, action):
        graph_path = self.graphs[self.current_graph_idx]
        
        # Decode action
        par = self.par_opts[action[0]]
        c4 = self.c4_opts[action[1]]
        map_opt = self.map_opts[action[2]]
        alloc = self.alloc_opts[action[3]]
        sched = self.sched_opts[action[4]]
        
        # Setup environment variables for OpenMP
        env_vars = os.environ.copy()
        env_vars["OMP_SCHEDULE"] = sched
        # Could also add num_threads to the action space
        
        cmd = [
            self.bin_path, 
            "--par", par, 
            "--c4", c4, 
            "--map", map_opt, 
            "--alloc", alloc, 
            graph_path, 
            "/dev/null"
        ]
        
        try:
            # We enforce a timeout because bad configs (like ARRAY on sparse graphs) take too long
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=15.0, env=env_vars)
            if result.returncode != 0:
                reward = -10.0 # Error penalty
            else:
                # The output contains the execution times (e.g., "0.1,0.5,0.2,0.1\n")
                last_line = result.stdout.strip().split("\n")[-1]
                times = list(map(float, last_line.split(",")))
                total_time = sum(times)
                # Reward is inverse of execution time (faster is higher reward)
                reward = 1.0 / (total_time + 1e-6)
        except subprocess.TimeoutExpired:
            # Bad configuration took too long
            reward = -5.0
        except Exception as e:
            print(f"Execution failed: {e}")
            reward = -10.0

        # Terminate immediately after one step (Contextual Bandit formulation)
        terminated = True
        truncated = False
        
        # Dummy next state since episode is over
        next_state = np.zeros(7, dtype=np.float32)
        
        return next_state, reward, terminated, truncated, {}

if __name__ == "__main__":
    if not gym:
        print("Cannot run training without ML libraries.")
        exit(1)
        
    print("Initializing NI-ORCA RL Environment...")
    env = NIORCAEnv()
    
    # Optional: Check if environment is valid
    # check_env(env)
    
    print("Building PPO Model...")
    # Multi-Layer Perceptron Policy
    model = PPO("MlpPolicy", env, verbose=1, tensorboard_log="./tensorboard_logs/")
    
    print("Training Model...")
    # NOTE: Training requires actual compiled binaries and generated data.
    # Uncomment to train!
    # model.learn(total_timesteps=5000)
    
    print("Training loop setup complete. Ensure graphs are generated and binary is built before calling model.learn()")
