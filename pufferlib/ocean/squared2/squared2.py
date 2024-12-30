'''A simple sample environment. Use this as a template for your own envs.'''

import gymnasium
import numpy as np

import pufferlib
from pufferlib.ocean.squared2.cy_squared2 import CySquared2


class Squared2(pufferlib.PufferEnv):
    def __init__(self, num_envs=1, render_mode=None, grid_size=11, player_to_move=1, buf=None):
        self.single_observation_space = gymnasium.spaces.Box(low=0, high=1,
            shape=(grid_size * grid_size * 2,), dtype=np.int32)
        self.single_action_space = gymnasium.spaces.Discrete(grid_size * grid_size * 2)
        self.render_mode = render_mode
        self.num_agents = num_envs

        super().__init__(buf)
        self.c_envs = CySquared2(self.observations, self.actions,
            self.rewards, self.terminals, num_envs, grid_size, player_to_move=1)
 
    def reset(self, seed=None):
        self.c_envs.reset()
        return self.observations, []

    def step(self, actions):
        self.actions[:] = actions
        self.c_envs.step()

        episode_returns = self.rewards[self.terminals]

        info = []
        if len(episode_returns) > 0:
            info = [{
                'reward': np.mean(episode_returns),
            }]

        return (self.observations, self.rewards,
            self.terminals, self.truncations, info)

    def render(self):
        self.c_envs.render()

    def close(self):
        self.c_envs.close()

def test_env(num_envs=1, grid_size=5, player_to_move=1):
    env = Squared2(grid_size=grid_size, player_to_move=player_to_move)
    env.reset()
    print(env.observations)
    while True:
        env.render()
        possible_moves, num_valid_moves = env.c_envs.get_valid_moves()
        print(num_valid_moves, )
        action_idx = np.random.randint(num_valid_moves[0])
        action_idx = np.array([np.random.randint(nvm) for nvm in num_valid_moves], dtype=np.int32)
        action = possible_moves[np.arange(num_envs), action_idx]
        # action = env.action_space.sample()
        obs, rew, terms, trunc, info = env.step(action)
        print(rew, terms, trunc, info)
        # if terms or trunc:
        #     break

def test_performance(timeout=10,):
    num_envs=1000;
    env = Squared2(num_envs=num_envs, grid_size=5, player_to_move=1)
    env.reset()
    tick = 0

    # actions = np.random.randint(0, env.single_action_space.n, (atn_cache, num_envs))

    import time
    start = time.time()
    while time.time() - start < timeout:
        possible_moves, num_valid_moves = env.c_envs.get_valid_moves()
        # print(num_valid_moves, )
        action_idx = np.array([np.random.randint(nvm) for nvm in num_valid_moves], dtype=np.int32)
        action = possible_moves[np.arange(num_envs), action_idx]
        # atn = actions[tick % atn_cache]
        env.step(action)
        tick += 1

    sps = num_envs * tick / (time.time() - start)
    print(f'SPS: {sps:,}')

if __name__ == '__main__':
    # test_env(grid_size=5)
    test_performance()
