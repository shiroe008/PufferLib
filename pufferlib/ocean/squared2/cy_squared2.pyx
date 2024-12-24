cimport numpy as cnp
from libc.stdlib cimport calloc, free

cdef extern from "squared2.h":
    ctypedef struct Squared2:
        unsigned char* observations
        int* actions
        float* rewards
        unsigned char* terminals
        int size
        int tick
        int r
        int c

    ctypedef struct Client

    void reset(Squared2* env)
    void step(Squared2* env)

    Client* make_client(Squared2* env)
    void close_client(Client* client)
    void render(Client* client, Squared2* env)

cdef class CySquared2:
    cdef:
        Squared2* envs
        Client* client
        int num_envs
        int size

    def __init__(self, unsigned char[:, :] observations, int[:] actions,
            float[:] rewards, unsigned char[:] terminals, int num_envs, int size):

        self.envs = <Squared2*> calloc(num_envs, sizeof(Squared2))
        self.num_envs = num_envs
        self.client = NULL

        cdef int i
        for i in range(num_envs):
            self.envs[i] = Squared2(
                observations = &observations[i, 0],
                actions = &actions[i],
                rewards = &rewards[i],
                terminals = &terminals[i],
                size=size,
            )

    def reset(self):
        cdef int i
        for i in range(self.num_envs):
            reset(&self.envs[i])

    def step(self):
        cdef int i
        for i in range(self.num_envs):
            step(&self.envs[i])

    def render(self):
        cdef Squared2* env = &self.envs[0]
        if self.client == NULL:
            self.client = make_client(env)

        render(self.client, env)

    def close(self):
        if self.client != NULL:
            close_client(self.client)
            self.client = NULL

        free(self.envs)
