#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"

const int INVALID_TILE = 3;
const int EMPTY = 0;
const int PLAYER1 = 1;
const int PLAYER2 = 2;

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
    int rows;
    int cols;
    int total_tiles;
    int* possible_moves;
    int* possible_moves_idx;
    int num_empty_tiles;
    unsigned char player_to_move;
    int* visited;
    Group* p1;
    Group* p2;
    int edge1;
    int edge2;
    Group reach_top;
    Group reach_bot;
    Group reach_left;
    Group reach_right;
};

void generate_board_positions(Hex* env) {
    for (int row = 0; row < env->rows; row++) {
        for (int col = 0; col < env->cols; col++) {
            if ((row+col) % 2 != 0) {
                env->observations[row * env->cols + col] = INVALID_TILE;
            }
        }
    }
}

void init_groups(Hex* env) {
    for (int i = 0; i < env->total_tiles; i++) {
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
    for (int i = 0; i < env-> rows; i++) {
        if (i % 2 == 0) {
            cell = i * env->cols;
            union_groups(env->p1, env->edge1, cell);
        } else {
            cell = i * env->cols + 1;
            union_groups(env->p1, env->edge1, cell);
        }
    }

    for (int i = 0; i < env->rows; i++) {
        if (i % 2 == 0) {
            cell = i * env->cols + env->cols - 2;
            union_groups(env->p1, env->edge2, cell);
        } else {
            cell = i * env->cols + env->cols - 1;
            union_groups(env->p1,  env->edge2, cell);
        }
    }

    for (int i = 0; i < env->cols; i+=2) {
        union_groups(env->p2, env->edge1, i);
    }

    if (env->rows % 2 == 1) {
        for (int i = 0; i < env->cols; i+=2) {
            cell = i + env->cols * (env->rows - 1);
            union_groups(env->p2, env->edge2, cell);
        }
    } else {
        for (int i = 1; i < env->cols; i+=2) {
            cell = i + env->cols * (env->rows - 1);
            union_groups(env->p2, env->edge2, cell);
        }
    }
}

int get_possible_moves(Hex* env){
    memset(env->possible_moves, 0, env->num_empty_tiles * sizeof(int));
    int count = 0;

    for(int i = 0; i < env->total_tiles; i++) {
        if(env->observations[i] == EMPTY){
            env->possible_moves_idx[i] = count;
            env->possible_moves[count++] = i;
        } else {
            env->possible_moves_idx[i] = -1;
        }
    }
    return count;
}

void init(Hex* env) {
    env->player_to_move = PLAYER1;
    env->rows = env->grid_size, env->cols = env->grid_size * 2;
    env->total_tiles = env->rows * env->cols;
    env->num_empty_tiles = env->total_tiles / 2;
    env->possible_moves = (int*)calloc(env->num_empty_tiles, sizeof(int));
    env->possible_moves_idx = (int*)calloc(env->total_tiles, sizeof(int));
    env->visited = (int*)calloc(env->total_tiles, sizeof(int));
    env->edge1 = env->total_tiles;
    env->edge2 = env->edge1 + 1;
    env->p1 = (Group*)calloc((env->total_tiles+2), sizeof(Group));
    env->p2 = (Group*)calloc((env->total_tiles+2), sizeof(Group));
}

void allocate(Hex* env) {
    init(env);
    env->observations = (int*)calloc(env->total_tiles, sizeof(int));
    env->actions = (int*)calloc(1, sizeof(int));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_initialized(Hex* env) {
    free(env->visited);
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
    memset(env->observations, EMPTY, env->total_tiles * sizeof(int));
    generate_board_positions(env);
    env->num_empty_tiles = get_possible_moves(env);
    memset(env->visited, 0, env->total_tiles * sizeof(int));
    env->terminals[0] = 0;
    env->rewards[0] = 0;
    init_groups(env);
}

int get_neighbors(Hex* env, int pos, int* neighbors){
    int row = pos / env->cols;
    int col = pos % env->cols;
    int count = 0;
    
    if ((row % 2) != (col % 2)) {
        return 0;  
    }

    if (row % 2 == 1) {
        const int dr[] = {-1, -1, 0, 0, 1, 1};
        const int dc[] = {-1, 1, -2, 2, -3, -1};
    
        for (int i = 0; i < 6; i++) {
            int new_row = row + dr[i];
            int new_col = col + dc[i];
            int new_pos = new_row * env->cols + new_col;
            if (new_row >= 0 && new_row < env->rows && 
                new_col >= 0 && new_col < env->cols && 
                (new_row % 2) == (new_col % 2)) {
                neighbors[count++] = new_pos;
            }
        } 
    }
    else {
        const int dr[] = {-1, -1, 0, 0, 1, 1};
        const int dc[] = {3, 1, -2, 2, -1, 1};
    
        for (int i = 0; i < 6; i++) {
            int new_row = row + dr[i];
            int new_col = col + dc[i];
            int new_pos = new_row * env->cols + new_col;
            if (new_row >= 0 && new_row < env->rows &&
                new_col >= 0 && new_col < env->cols &&
                (new_row % 2) == (new_col % 2)) {
                neighbors[count++] = new_pos;
            }
        }
    }
    return count;
}

void check_win_uf(Hex* env, int player, int pos){
    Group* groups = (player == PLAYER1) ? env->p1 : env->p2;
    int neighbors[6];
    int num_neighbors = get_neighbors(env, pos, neighbors);
    for (int i = 0; i < num_neighbors; i++) {
        if (env->observations[neighbors[i]] == player) {
            union_groups(groups, neighbors[i], pos);
        }
    }
    
    if (find(groups, env->edge1) == find(groups, env->edge2)) {
        reset(env);
        env->terminals[0] = 1;
        env->rewards[0] = (player == PLAYER1) ? 1.0 : -1.0;
    }
}

int can_make_move(Hex* env, int pos, int player){
    // cannot place stone on occupied tile
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
    
    for (int row = 0; row < env->rows; row++) {
        for (int col = 0; col < env->cols; col++) {
            int tile_type= env->observations[row * env->cols + col];
            if (tile_type == INVALID_TILE) {
                continue;
            }
            else {
                Color color;
                if (tile_type == EMPTY) {
                    color = PUFF_WHITE;
                }
                else if (tile_type == PLAYER1) {
                    color = PUFF_RED;
                } else if (tile_type == PLAYER2) {
                    color = PUFF_CYAN;
                }
                if (row % 2 == 0) {
                    DrawPoly((Vector2){200 + cos30 * (row+col) * radius,
                            200 + row * radius * 1.5}, 6, radius, 90, color);
                    DrawPolyLines((Vector2){200 + cos30 * (row+col) * radius,
                            200 + row * radius * 1.5}, 6, radius, 90, STONE_GRAY);
                }
                else {
                    DrawPoly((Vector2){200 + cos30 * ((2*col-1)/2 + row) * radius,
                            200 + row * radius * 1.5}, 6, radius, 90, color);
                    DrawPolyLines((Vector2){200 + cos30 * ((2*col-1)/2 + row) * radius,
                            200 + row * radius * 1.5}, 6, radius, 90, STONE_GRAY);
                }
            }
        }
    }
    EndDrawing();
}
