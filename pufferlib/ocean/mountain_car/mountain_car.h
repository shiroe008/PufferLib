#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

#define MIN_POSITION -1.2f;
#define MAX_POSITION 0.6f;
#define MAX_SPEED 0.07f;
#define GOAL_POSITION 0.5f;
#define GOAL_VELOCITY 0.0f;

#define GRAVITY 0.0025f;
#define FORCE 0.001f;

#define WIDTH 600.0f;
#define HEIGHT 400.0f;

typedef struct Log Log;
struct Log {
    float perf;
    float episode_length;
    float n;
    float score;
};

typedef struct Client Client;
struct Client {
};

typedef struct MountainCar MountainCar;
struct MountainCar {
    float* observations;
    float* actions;
    float* rewards;
    unsigned char* terminals;
    unsigned char* truncations;
    Log log;
    Client* client;
    int tick;
    float episode_return;
    float x;
    float x_dot;
};

void add_log(MountainCar* env) {
    if (env->episode_return > 0) {
        env->log.perf = env->episode_return / MAX_STEPS;
    } else {
        env->log.perf = 0.0f;
    }
    env->log.episode_length += env->tick;
    env->log.score += env->tick;
    env->log.n += 1;
}

void init(MountainCar* env) {
    env->tick = 0;
    memset(&env->log, 0, sizeof(Log));
}

void allocate(MountainCar* env) {
    init(env);
    env->observations = (float*)calloc(1, sizeof(float));
    env->actions = (float*)calloc(1, sizeof(float));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_allocated(MountainCar* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void c_close(MountainCar* env) {
}

const Color PUFF_RED = (Color){187, 0, 0, 255};
const Color PUFF_CYAN = (Color){0, 187, 187, 255};
const Color PUFF_WHITE = (Color){241, 241, 241, 241};
const Color PUFF_BACKGROUND = (Color){6, 24, 24, 255};

Client* make_client(MountainCar* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    InitWindow(WIDTH, HEIGHT, "puffer MountainCar");
    SetTargetFPS(60);
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

void c_render(MountainCar* env) {
    if (IsKeyDown(KEY_ESCAPE))
        exit(0);
    if (IsKeyPressed(KEY_TAB))
        ToggleFullscreen();

    if (env->client == NULL) {
        env->client = make_client(env);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);
    DrawText(TextFormat("Steps: %i", env->tick), 10, 10, 20, PUFF_WHITE);
    EndDrawing();
}


