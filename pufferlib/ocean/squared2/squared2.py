'''A simple sample environment. Use this as a template for your own envs.'''

import gymnasium
import numpy as np

import pufferlib
from pufferlib.ocean.squared2.cy_squared2 import CySquared2


class Squared2(pufferlib.PufferEnv):
    def __init__(self, num_envs=1, render_mode=None, size=11, buf=None):
        self.single_observation_space = gymnasium.spaces.Box(low=0, high=1,
            shape=(size*size,), dtype=np.uint8)
        self.single_action_space = gymnasium.spaces.Discrete(5)
        self.render_mode = render_mode
        self.num_agents = num_envs

        super().__init__(buf)
        self.c_envs = CySquared2(self.observations, self.actions,
            self.rewards, self.terminals, num_envs, size)
 
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

def test_env():
    env = Squared2()
    env.reset()
    print(env.observation_space)
    while True:
        env.render()
        action = env.action_space.sample()
        obs, rew, terms, trunc, info = env.step(action)
        print(obs, rew, terms, trunc, info)
        if terms or trunc:
            break

if __name__ == '__main__':
    test_env()
