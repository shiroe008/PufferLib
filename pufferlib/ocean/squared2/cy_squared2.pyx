cimport numpy as cnp
import numpy as np
from libc.stdlib cimport calloc, free

cdef extern from "squared2.h":
    ctypedef struct Squared2:
        int* observations;
        int* actions;
        float* rewards;
        unsigned char* terminals;
        int grid_size;
        int rows;
        int cols;
        int total_tiles;
        int* possible_moves;
        int num_empty_tiles;
        int player_to_move;
        int* board_states;
        int* visited;

    ctypedef struct Client

    void reset(Squared2* env)
    void step(Squared2* env)
    void init(Squared2* env)
    int get_posible_moves(Squared2* env)

    Client* make_client(Squared2* env)
    void close_client(Client* client)
    void render(Client* client, Squared2* env)

cdef class CySquared2:
    cdef:
        Squared2* envs
        Client* client
        int num_envs
        int grid_size
        object possible_moves
        object num_valid_moves

    def __init__(self, int[:, :] observations, int[:] actions, float[:] rewards, unsigned char[:] terminals, 
                 int num_envs, int grid_size, int player_to_move):

        self.envs = <Squared2*> calloc(num_envs, sizeof(Squared2))
        self.num_envs = num_envs
        self.client = NULL

        self.num_valid_moves = np.zeros(num_envs, dtype=np.int32)
        self.possible_moves = np.zeros((num_envs, grid_size * grid_size), dtype=np.int32)

        cdef int i
        for i in range(num_envs):
            self.envs[i] = Squared2(
                observations = &observations[i, 0],
                actions = &actions[i],
                rewards = &rewards[i],
                terminals = &terminals[i],
                grid_size = grid_size,
                player_to_move = player_to_move,
            )
            init(&self.envs[i])
            self.client = NULL

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

    def get_valid_moves(self):
        cdef int i, j
        for i in range(self.num_envs):
            self.num_valid_moves[i] = get_posible_moves(&self.envs[i])
            for j in range(self.num_valid_moves[i]):
                self.possible_moves[i, j] = self.envs[i].possible_moves[j]

        return self.possible_moves, self.num_valid_moves
