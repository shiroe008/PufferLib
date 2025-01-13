#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"

const int INVALID_TILE = 3;
const int EMPTY = 0;
const int PLAYER1 = 1;
const int PLAYER2 = 2;
#define NUM_DIRECTIONS 6
static const int DIRECTIONS[NUM_DIRECTIONS][2] = {{0, -1}, {0, 1}, {-1, 0}, {-1, 1}, {1, -1}, {1, 0}};

typedef struct Group Group;
struct Group {
    int parent;
    int size;
};

int find(Group* groups, int x) {
    if (groups[x].parent != x)
        groups[x].parent = find(groups, groups[x].parent);
    return groups[x].parent;
}

void union_groups(Group* groups, int pos1, int pos2) {
    pos1 = find(groups, pos1);
    pos2 = find(groups, pos2);
    
    if (pos1 == pos2) return;
    
    if (groups[pos1].size < groups[pos2].size) {
        groups[pos1].parent = pos2;
        groups[pos2].size += groups[pos1].size;
    } else {
        groups[pos2].parent = pos1;
        groups[pos1].size += groups[pos2].size;
    }
}
 
typedef struct Hex Hex;
struct Hex {
    int* observations;
    int* actions;
    float* rewards;
    unsigned char* terminals;
    int grid_size;
    int* possible_moves;
    int* possible_moves_idx;
    int num_empty_tiles;
    unsigned char player_to_move;
    Group* p1;
    Group* p2;
    int edge1;
    int edge2;
};

void init_groups2(Hex* env) {
    for (int i = 0; i < env->grid_size * env->grid_size; i++) {
        env->p1[i].parent = i;
        env->p1[i].size = 1;
        env->p2[i].parent = i;
        env->p2[i].size = 1;
    }
    env->p1[env->edge1].parent = env->edge1;
    env->p1[env->edge1].size = 1;
    env->p1[env->edge2].parent = env->edge2;
    env->p1[env->edge2].size = 1;
    env->p2[env->edge1].parent = env->edge1;
    env->p2[env->edge1].size = 1;
    env->p2[env->edge2].parent = env->edge2;
    env->p2[env->edge2].size = 1;

    int cell;

    for (int i = 0; i < env->grid_size; i++) {
        cell = i * env->grid_size;
        union_groups(env->p1, env->edge1, cell);
        
        cell = i * env->grid_size + env->grid_size - 1;
        union_groups(env->p1, env->edge2, cell);

        cell = i;
        union_groups(env->p2, env->edge1, cell);

        cell = i + env->grid_size * (env->grid_size - 1);
        union_groups(env->p2, env->edge2, cell);
    }
}


int get_possible_moves(Hex* env){
    memset(env->possible_moves, 0, env->num_empty_tiles * sizeof(int));
    int count = 0;

    for(int i = 0; i < env->grid_size * env->grid_size; i++) {
        if(env->observations[i] == EMPTY){
            env->possible_moves_idx[i] = count;
            env->possible_moves[count++] = i;
        }
    }
    return count;
}

void init(Hex* env) {
    env->num_empty_tiles = env->grid_size * env->grid_size;
    env->possible_moves = (int*)calloc(env->num_empty_tiles, sizeof(int));
    env->possible_moves_idx = (int*)calloc(env->num_empty_tiles, sizeof(int));
    env->edge1 = env->grid_size * env->grid_size;
    env->edge2 = env->edge1 + 1;
    env->p1 = (Group*)calloc((env->grid_size * env->grid_size + 2), sizeof(Group));
    env->p2 = (Group*)calloc((env->grid_size * env->grid_size + 2), sizeof(Group));
}

void allocate(Hex* env) {
    init(env);
    env->observations = (int*)calloc(env->grid_size * env->grid_size, sizeof(int));
    env->actions = (int*)calloc(1, sizeof(int));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_initialized(Hex* env) {
    free(env->possible_moves);
    free(env->possible_moves_idx);
    free(env->p1);
    free(env->p2);
}

void free_allocated(Hex* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
    free_initialized(env);
}

void reset(Hex* env) {
    env->player_to_move = PLAYER1;
    memset(env->observations, EMPTY, env->grid_size * env->grid_size * sizeof(int));
    env->num_empty_tiles = get_possible_moves(env);
    env->terminals[0] = 0;
    env->rewards[0] = 0;
    init_groups2(env);
}

void check_win_uf(Hex* env, int player, int pos){
    Group* groups = (player == PLAYER1) ? env->p1 : env->p2;
    int row = pos / env->grid_size;
    int col = pos % env->grid_size;

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        int row_neigh = row + DIRECTIONS[i][0];
        int col_neigh = col + DIRECTIONS[i][1];
        int pos_neigh = row_neigh * env->grid_size + col_neigh;
        if (row_neigh < 0 || row_neigh >= env->grid_size || 
            col_neigh < 0 || col_neigh >= env->grid_size || 
            env->observations[pos_neigh] != player) {
            continue;
        }
        union_groups(groups, pos_neigh, pos);
    }

    if (find(groups, env->edge1) == find(groups, env->edge2)) {
        reset(env);
        env->terminals[0] = 1;
        env->rewards[0] = (player == PLAYER1) ? 1.0 : -1.0;
    }
}

int can_make_move(Hex* env, int pos, int player){
    if (env->observations[pos] != EMPTY) {
        return 0;
    }
    env->observations[pos] = player;
    return 1;
}

void update_possible_moves(Hex* env, int action){
    int move_idx = env->possible_moves_idx[action];
    int last_available_move = env->possible_moves[env->num_empty_tiles - 1];
    env->possible_moves[move_idx] = last_available_move;
    env->possible_moves_idx[last_available_move] = move_idx;
    env->num_empty_tiles--;
}

void make_random_move(Hex* env, int player) {
    int count = get_possible_moves(env);
    for(int i = count - 1; i > 0; i--){
        int j = rand() % (i + 1);
        int temp = env->possible_moves[i];
        env->possible_moves[i] = env->possible_moves[j];
        env->possible_moves[j] = temp;
    }
    can_make_move(env, env->possible_moves[0], player);
}

void step(Hex* env) {
    int action = env->actions[0];
    env->terminals[0] = 0;
    env->rewards[0] = 0;
    if (can_make_move(env, action, env->player_to_move)) {
        update_possible_moves(env, action);
        check_win_uf(env, env->player_to_move, action);
        env-> player_to_move = env->player_to_move ^ 3;
    }
}

const Color STONE_GRAY = (Color){80, 80, 80, 255};
const Color PUFF_RED = (Color){187, 0, 0, 255};
const Color PUFF_CYAN = (Color){0, 187, 187, 255};
const Color PUFF_WHITE = (Color){241, 241, 241, 241};
const Color PUFF_BACKGROUND = (Color){6, 24, 24, 255};
const Color PUFF_BACKGROUND2 = (Color){18, 72, 72, 255};

typedef struct Client Client;
struct Client {
    Texture2D ball;
};

Client* make_client(Hex* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    int px = 128*env->grid_size;
    InitWindow(px, px, "PufferLib Hex");
    SetTargetFPS(1);

    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

void render(Client* client, Hex* env) {
    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);

    float radius = 20.0;
    float cos30 = cos(30 * M_PI / 180);
    Color color;
    
    for (int row = 0; row < env->grid_size; row++) {
        for (int col = 0; col < env->grid_size; col++) {
            int tile_type= env->observations[row * env->grid_size + col];
            if (tile_type == EMPTY) {
                color = PUFF_WHITE;
            }
            else if (tile_type == PLAYER1) {
                color = PUFF_RED;
            } else if (tile_type == PLAYER2) {
                color = PUFF_CYAN;
            }
            DrawPoly((Vector2){200 + cos30 * (2 * col + row) * radius,
                    200 + row * radius * 1.5}, 6, radius, 90, color);
            DrawPolyLines((Vector2){200 + cos30 * (2 * col + row) * radius,
                    200 + row * radius * 1.5}, 6, radius, 90, STONE_GRAY);
        }
    }
    EndDrawing();
}
