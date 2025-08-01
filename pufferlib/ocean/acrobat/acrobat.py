import numpy as np
import gymnasium
import pufferlib
from pufferlib.ocean.acrobat import binding

class Acrobat(pufferlib.PufferEnv):
    def __init__(self, num_envs=1, render_mode='human', report_interval=1, buf=None, seed=0):
        self.render_mode = render_mode
        self.num_agents = num_envs
        self.report_interval = report_interval
        self.tick = 0
        self.human_action = None

        self.num_obs = 6
        self.single_observation_space = gymnasium.spaces.Box(
            low=-np.inf, high=np.inf, shape=(self.num_obs,), dtype=np.float32
        )

        self.single_action_space = gymnasium.spaces.Discrete(3)

        super().__init__(buf)
        self.actions = np.zeros(num_envs, dtype=np.float32)

        self.c_envs = binding.vec_init(
            self.observations,
            self.actions,
            self.rewards,
            self.terminals,
            self.truncations,
            num_envs,
            seed,
        )

    def reset(self, seed=None):
        self.tick = 0      
        if seed is None:
            binding.vec_reset(self.c_envs, 0)
        else:
            binding.vec_reset(self.c_envs, seed)
        return self.observations, []

    def step(self, actions):
        self.actions[:] = np.clip(actions.flatten(), -2.0, 2.0)
        
        self.tick += 1    
        binding.vec_step(self.c_envs)
        
        info = []
        if self.tick % self.report_interval == 0:
            info.append(binding.vec_log(self.c_envs))
        
        return (
            self.observations,
            self.rewards,
            self.terminals,
            self.truncations,
            info
        )
   
    def render(self):
        binding.vec_render(self.c_envs, 0)
   
    def close(self):
        binding.vec_close(self.c_envs)

def test_performance(timeout=10, atn_cache=8192, continuous=True):
    """Benchmark environment performance."""
    num_envs = 4096
    env = Acrobat(num_envs=num_envs,)
    env.reset()
    tick = 0

    actions = np.random.randint(0, env.single_action_space.n, (atn_cache, num_envs)).astype(np.int8)

    import time
    start = time.time()
    while time.time() - start < timeout:
        atn = actions[tick % atn_cache]
        env.step(atn)
        tick += 1
    sps = num_envs * tick / (time.time() - start)
    print(f'SPS: {sps:,}')

if __name__ == '__main__':
    test_performance()
    
