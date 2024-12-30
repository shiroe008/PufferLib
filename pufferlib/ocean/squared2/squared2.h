#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"

const unsigned char NOOP = 0;
const unsigned char DOWN = 1;
const unsigned char UP = 2;
const unsigned char LEFT = 3;
const unsigned char RIGHT = 4;

const int INVALID_TILE = 3;
const int EMPTY = 0;
const int PLAYER1 = 1;
const int PLAYER2 = 2;

const unsigned char AGENT = 1;
const unsigned char TARGET = 2;
 
typedef struct Squared2 Squared2;
struct Squared2 {
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
    unsigned char player_to_move;
    int* board_states;
    int* visited;
};

void generate_board_positions(Squared2* env) {
    // comment the line below when running in python
    // env->observations = (int*)calloc(env->total_tiles, sizeof(int));
    for (int row = 0; row < env->rows; row++) {
        for (int col = 0; col < env->cols; col++) {
            if ((row+col) % 2 != 0) {
                env->observations[row * env->cols + col] = INVALID_TILE;
                // env->board_states[row * env->cols + col] = INVALID_TILE;
                // memcpy(env->observations[row * env->cols + col], env->board_states[row * env->cols + col], sizeof(int));
            }
        }
    }
}

void init(Squared2* env) {
    env->player_to_move = PLAYER1;
    env->rows = env->grid_size, env->cols = env->grid_size * 2;
    env->total_tiles = env->rows * env->cols;
    env->possible_moves = (int*)calloc(env->total_tiles, sizeof(int));
    env->visited = (int*)calloc(env->total_tiles, sizeof(int));
    env->board_states = (int*)calloc(env->total_tiles, sizeof(int));
    generate_board_positions(env);
}

void allocate(Squared2* env) {
    env->observations = (int*)calloc(env->total_tiles, sizeof(int));
    env->actions = (int*)calloc(1, sizeof(int));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
    init(env);
    // generate_board_positions(env);
}

void free_allocated(Squared2* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void reset(Squared2* env) {
    memset(env->observations, EMPTY, env->total_tiles * sizeof(int));
    memset(env->board_states, EMPTY, env->total_tiles * sizeof(int));
    env->num_empty_tiles = get_posible_moves(env);
    generate_board_positions(env);
    memset(env->visited, 0, env->total_tiles * sizeof(int));
    env->terminals[0] = 0;
    env->rewards[0] = 0;
}

int get_neighbors(Squared2* env, int pos, int* neighbors){
    int row = pos / env->cols;
    int col = pos % env->cols;
    int count = 0;
    
    // Check if current position is valid hex position
    // Even rows have hexes at even columns, odd rows at odd columns
    if ((row % 2) != (col % 2)) {
        return 0;  // Invalid hex position
    }

    if (row % 2 == 1) {
        // Neighbor offsets for both even and odd rows
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
            // Check bounds and ensure new position is valid hex position
            if (new_row >= 0 && new_row < env->rows &&
                new_col >= 0 && new_col < env->cols &&
                (new_row % 2) == (new_col % 2)) {
                neighbors[count++] = new_pos;
            }
        }
    }
    return count;
}

void dfs(Squared2* env, int pos, int player){
    int curr_row = pos / env->cols;
    int curr_col = pos % env->cols;

    if (player == PLAYER1){
        if ((curr_col == env->cols - 1 && curr_row % 2 == 1) || 
            (curr_col== env->cols - 2 && curr_row % 2 == 0)){
            // printf("player 1 wins\n");
            reset(env);
            env->terminals[0] = 1;
            env->rewards[0] = 1.0;
            return;
        }
    }
    else if (player == PLAYER2){
        if (curr_row == env->rows - 1){
            // printf("player 2 wins\n");
            reset(env);
            env->terminals[0] = 1;
            env->rewards[0] = -1.0;
            return;
        }
    }
    env->visited[pos] = player;

    int neighbors[6];
    int num_neighbors = get_neighbors(env, pos, neighbors);
    for (int i = 0; i < num_neighbors; i++) {
        int neighbor = neighbors[i];
        if (env->observations[neighbor] == player && env->visited[neighbor] != player){
            env->visited[neighbor] = player;
            dfs(env, neighbor, player);
        }
    }
    return;
}

void check_win(Squared2* env, int player, int possible_moves){
    if (!possible_moves) {
        // printf("MATHEMATICALLY A DRAW IS NOT POSSIBLE\n");
        reset(env);
        env->terminals[0] = 1;
        env->rewards[0] = 0.0;
        return;
    }

    memset(env->visited, 0, env->total_tiles * sizeof(int));
    if (player == PLAYER1){
        for (int row = 0; row < env->rows; row++) {
            int cell;
            if (row % 2 == 0) {
                cell = row * env->cols;
            }
            else {
                cell = row * env->cols + 1;
            }
            if (env->observations[cell] == PLAYER1) {
                dfs(env, cell, player);
               }
            }
    }
    
    else if (player == PLAYER2){
        for (int col = 0; col < env->cols; col+=2) {
            int cell = col;
            if (env->observations[cell] == PLAYER2) {
                dfs(env, cell, player);
            }
        }
    }
}

void make_move(Squared2* env, int pos, int player){
    // cannot place stone on occupied tile
    if (env->observations[pos] != EMPTY) {
        return;
    }
    else {
        env->observations[pos] = player;
    }
}

int get_posible_moves(Squared2* env){
    memset(env->possible_moves, 0, env->total_tiles * sizeof(int));
    int count = 0;

    for(int i = 0; i < env->total_tiles; i++){
        if(env->observations[i] == EMPTY){
            env->possible_moves[count++] = i;
        }
    }
    return count;
}

void make_random_move(Squared2* env, int player) {
    int count = get_posible_moves(env);
    for(int i = count - 1; i > 0; i--){
        int j = rand() % (i + 1);
        int temp = env->possible_moves[i];
        env->possible_moves[i] = env->possible_moves[j];
        env->possible_moves[j] = temp;
    }
    // Try to make a move in a random empty position
    make_move(env, env->possible_moves[0], player);
}

void step(Squared2* env) {
    env->num_empty_tiles = get_posible_moves(env);
    // int action_idx = rand() % env->num_empty_tiles;
    //int action = env->possible_moves[action_idx];
    int action = env->actions[0];
    env->terminals[0] = 0;
    env->rewards[0] = 0;
    make_move(env, action, env->player_to_move);
    check_win(env, PLAYER1, env->num_empty_tiles);
    check_win(env, PLAYER2, env->num_empty_tiles);
    env-> player_to_move = env->player_to_move ^ 3;
    //if (env->player_to_move == PLAYER1) {
    //    env->player_to_move = PLAYER2;
    //}
    //else {
    //    env->player_to_move = PLAYER1;
    //}   
    //make_random_move(env, env->player_to_move);
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

Client* make_client(Squared2* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    int px = 128*env->grid_size;
    InitWindow(px, px, "PufferLib Squared2");
    SetTargetFPS(5);

    //client->agent = LoadTexture("resources/puffers_128.png");
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

void render(Client* client, Squared2* env) {
    if (IsKeyDown(KEY_ESCAPE)) {
        exit(0);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);

    float radius = 20.0;
    float cos30 = cos(30 * M_PI / 180);
    
    //env->observations[2*env->cols + 0] = PLAYER1;
    //env->observations[2*env->cols + 2] = PLAYER1;
    //env->observations[0*env->cols + 4] = PLAYER2;
    //env->observations[1*env->cols + 5] = PLAYER2;
    //env->observations[2*env->cols + 4] = PLAYER2;
    //env->observations[3*env->cols + 5] = PLAYER2;
    //env->observations[4*env->cols + 4] = PLAYER2;
    //env->observations[2*env->cols + 6] = PLAYER1;
    //env->observations[2*env->cols + 8] = PLAYER1;

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
