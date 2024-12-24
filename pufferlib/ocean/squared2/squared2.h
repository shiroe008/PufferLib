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

const unsigned char EMPTY = 0;
const unsigned char AGENT = 1;
const unsigned char TARGET = 2;
 
typedef struct Squared2 Squared2;
struct Squared2 {
    unsigned char* observations;
    int* actions;
    float* rewards;
    unsigned char* terminals;
    int grid_size;
    int rows;
    int cols;
    int* board_states;
    int grid_square_size;
    int* board_x;
    int* board_y;
    int size;
    int tick;
    int r;
    int c;
};

void generate_board_positions(Squared2* env) {
    for (int i = 0; i < (env->grid_size-1) * (env->grid_size-1); i++) {
        int row = i / (env->grid_size-1);
        int col = i % (env->grid_size-1);
        env->board_x[i] = col * (env->grid_square_size-1);
        env->board_y[i] = row * (env->grid_square_size-1);
    }
    for (int i = 0; i < env->rows * env->cols; i++) {
        int row = i / env->rows;
        int col = i % env->cols;
        if ((row+col) % 2 != 0) {
            env->board_states[row * env->grid_size + col] = -1;
        }
    }
}

void init(Squared2* env) {
    int board_render_size = (env->grid_size-1)*(env->grid_size-1);
    env->rows = env->grid_size, env->cols = env->grid_size;
    int grid_size = env->rows * env->cols;
    env->board_states = (int*)calloc(grid_size, sizeof(int));
    
    if (!env->board_states) {
        printf("Memory allocation failed for board_states\n");
        exit(1);
    }
    printf("Memory allocated for board_states\n");
    
    env->board_x = (int*)calloc(board_render_size, sizeof(int));
    env->board_y = (int*)calloc(board_render_size, sizeof(int));
    generate_board_positions(env);

}

void allocate(Squared2* env) {
    init(env);
    env->observations = (unsigned char*)calloc(env->size*env->size, sizeof(unsigned char));
    env->actions = (int*)calloc(1, sizeof(int));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_allocated(Squared2* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void reset(Squared2* env) {
    memset(env->observations, 0, env->size*env->size*sizeof(unsigned char));
    env->observations[env->size*env->size/2] = AGENT;
    env->r = env->size/2;
    env->c = env->size/2;
    env->tick = 0;
    int target_idx;
    do {
        target_idx = rand() % (env->size*env->size);
    } while (target_idx == env->size*env->size/2);
    env->observations[target_idx] = TARGET;
}

void step(Squared2* env) {
    int action = env->actions[0];
    env->terminals[0] = 0;
    env->rewards[0] = 0;

    env->observations[env->r*env->size + env->c] = EMPTY;

    if (action == DOWN) {
        env->r += 1;
    } else if (action == RIGHT) {
        env->c += 1;
    } else if (action == UP) {
        env->r -= 1;
    } else if (action == LEFT) {
        env->c -= 1;
    }

    if (env->tick > 3*env->size 
            || env->r < 0
            || env->c < 0
            || env->r >= env->size
            || env->c >= env->size) {
        env->terminals[0] = 1;
        env->rewards[0] = -1.0;
        reset(env);
        return;
    }

    int pos = env->r*env->size + env->c;
    if (env->observations[pos] == TARGET) {
        env->terminals[0] = 1;
        env->rewards[0] = 1.0;
        reset(env);
        return;
    }

    env->observations[pos] = AGENT;
    env->tick += 1;
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
    int px = 64*env->size;
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
    for (int i = 0; i < env->rows * env->cols; i++) {
        int row = i / env->cols;
        int col = i % env->cols;

        if (env->board_states[row * env->grid_size + col] == -1) {
            continue;
        }
        else if (env->board_states[row * env->grid_size + col] == 0) {
            if (row % 2 == 0) {
                DrawPoly((Vector2){200 + cos(30 * M_PI / 180) * (row+col) * radius,
                        200 + col * radius * 1.5}, 6, radius, 90, PUFF_RED);
                DrawPolyLines((Vector2){200 + cos(30 * M_PI / 180) * (row+col) * radius,
                        200 + col * radius * 1.5}, 6, radius, 90, PUFF_WHITE);
            }
            else {
                DrawPoly((Vector2){200 + cos(30 * M_PI / 180) * ((2*col-1)/2 + row) * radius,
                        200 + row * radius * 1.5}, 6, radius, 90, PUFF_RED);
                DrawPolyLines((Vector2){200 + cos(30 * M_PI / 180) * ((2*col-1)/2 + row) * radius,
                        200 + row * radius * 1.5}, 6, radius, 90, PUFF_WHITE);
            }
        }
    }

    //for (int i = 0; i < env->size; i++) {
    //    DrawPoly((Vector2){2 * i * radius, 64 * env->size / 2}, 6, radius, 90, PUFF_RED);
    //}
    
//    int rows = env->grid_size, cols = env->grid_size;
//    int array[10][10] = {-1};
//    for (int j = 0; j < cols; j++) {
//        if (j == 0) {
//            for (int i = 0; i < rows; i+=2) {
//                array[i][j] = 0;
//            }
//        }
//        else {
//            for (int i = 1; i < rows; i+=2) {
//                array[i][j] = 0;
//            }
//        }
//    }
//
//
//    for (int j = 0; j < cols; j++) {
//        if (j % 2 == 0) {
//            for (int i = 0; i < rows; i+=2) {
//        DrawPoly((Vector2){200 + cos(30 * M_PI / 180) * (i+j) * radius,
//                200 + j * radius * 1.5}, 6, radius, 90, PUFF_RED);
//        DrawPolyLines((Vector2){200 + cos(30 * M_PI / 180) * (i+j) * radius,
//                200 +j * radius * 1.5}, 6, radius, 90, PUFF_WHITE);
//            }
//        }
//        else {
//            for (int i = 1; i < rows; i+=2) {
//        DrawPoly((Vector2){200 + cos(30 * M_PI / 180) * ((2*i-1)/2 + j) * radius,
//                200 + j * radius * 1.5}, 6, radius, 90, PUFF_RED);
//        DrawPolyLines((Vector2){200 + cos(30 * M_PI / 180) * ((2*i-1)/2 + j) * radius,
//                200 + j * radius * 1.5}, 6, radius, 90, PUFF_WHITE);
//            }
//        }
//    }
//
    // int px = 64;
    // for (int i = 0; i < env->size; i++) {
    //    for (int j = 0; j < env->size; j++) {
    //        int tex = env->observations[i*env->size + j];
    //        if (tex == EMPTY) {
    //            continue;
    //        }
    //        Color color = (tex == AGENT) ? (Color){0, 127, 127, 255} : (Color){255, 0, 0, 255};
    //        DrawRectangle(j*px, i*px, px, px, color);
    //    }
    //}
    EndDrawing();
}
