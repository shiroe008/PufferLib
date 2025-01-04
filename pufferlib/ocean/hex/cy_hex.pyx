cimport numpy as cnp
import numpy as np
from libc.stdlib cimport calloc, free

cdef extern from "hex.h":
    ctypedef struct Group:
        int parent
        int size
    
    int find(Group*)
    void union_groups(Group* int, int)

    ctypedef struct Hex:
        int* observations;
        int* actions;
        float* rewards;
        unsigned char* terminals;
        int grid_size;
        int rows;
        int cols;
        int total_tiles;
        int* possible_moves;
        int* possible_moves_idx;
        int num_empty_tiles;
        unsigned char player_to_move;
        int* board_states;
        int* visited;
        Group* p1;
        Group* p2;
        int edge1;
        int edge2;
        Group reach_top;
        Group reach_bot;
        Group reach_left;
        Group reach_right;

    ctypedef struct Client

    void reset(Hex* env)
    void step(Hex* env)
    void init(Hex* env)

    Client* make_client(Hex* env)
    void close_client(Client* client)
    void render(Client* client, Hex* env)

cdef class CyHex:
    cdef:
        Hex* envs
        Client* client
        int num_envs
        int grid_size
        # object possible_moves
        # object num_valid_moves
        int [:, :] possible_moves
        int [:, :] possible_moves_idx
        int [:] num_valid_moves

    def __init__(self, int[:, :] observations, int[:] actions, float[:] rewards, unsigned char[:] terminals, 
                 int num_envs, int grid_size,):

        self.envs = <Hex*> calloc(num_envs, sizeof(Hex))
        self.num_envs = num_envs
        self.client = NULL

        self.num_valid_moves = np.zeros(num_envs, dtype=np.int32)
        self.possible_moves = np.zeros((num_envs, grid_size * grid_size), dtype=np.int32)
        self.possible_moves_idx = np.zeros((num_envs, grid_size * grid_size), dtype=np.int32)

        cdef int i
        for i in range(num_envs):
            self.envs[i] = Hex(
                observations = &observations[i, 0],
                actions = &actions[i],
                rewards = &rewards[i],
                terminals = &terminals[i],
                grid_size = grid_size,
            )
            init(&self.envs[i])
            self.envs[i].possible_moves = &self.possible_moves[i, 0]
            self.envs[i].possible_moves_idx = &self.possible_moves_idx[i, 0]
            self.envs[i].num_empty_tiles = self.num_valid_moves[i]
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
        cdef Hex* env = &self.envs[0]
        if self.client == NULL:
            self.client = make_client(env)

        render(self.client, env)

    def close(self):
        if self.client != NULL:
            close_client(self.client)
            self.client = NULL

        free(self.envs)

    def get_posible_moves(self):
        return self.c_envs.possible_moves, self.c_envs.num_empty_tiles

    def get_valid_moves(self):
        cdef int i;
        for i in range(self.num_envs):
            self.num_valid_moves[i] = self.envs[i].num_empty_tiles
            # for j in range(self.num_valid_moves[i]):
            #     self.possible_moves[i, j] = self.envs[i].possible_moves[j]

        return np.asarray(self.possible_moves), np.asarray(self.num_valid_moves)
