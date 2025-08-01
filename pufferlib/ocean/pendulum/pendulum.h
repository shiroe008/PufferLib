#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

#define GRAVITY 10.0f
#define MAX_SPEED 8.0f
#define MAX_TORQUE 2.0f
#define DT 0.05f
#define M 1.0f
#define L 1.0f

#define DEFAULT_THETA M_PI
#define DEFAULT_THETA_DOT 1.0

#define MAX_STEPS 200
#define WIDTH 500
#define HEIGHT 500
#define SCALE 100

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

typedef struct Pendulum Pendulum;
struct Pendulum {
    float* observations;
    float* actions;
    float* rewards;
    unsigned char* terminals;
    unsigned char* truncations;
    Log log;
    Client* client;
    int tick;
    float episode_return;
    float theta;
    float theta_dot;
};

void add_log(Pendulum* env) {
    if (env->episode_return > 0) {
        env->log.perf = env->episode_return / MAX_STEPS;
    } else {
        env->log.perf = 0.0f;
    }
    env->log.perf = env->episode_return;
    env->log.episode_length += env->tick;
    env->log.score += env->rewards[0];
    env->log.n += 1;
}

void init(Pendulum* env) {
    env->tick = 0;
    memset(&env->log, 0, sizeof(Log));
}

void allocate(Pendulum* env) {
    init(env);
    env->observations = (float*)calloc(3, sizeof(float));
    env->actions = (float*)calloc(1, sizeof(float));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_allocated(Pendulum* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void c_close(Pendulum* env) {
}

const Color PUFF_RED = (Color){187, 0, 0, 255};
const Color PUFF_CYAN = (Color){0, 187, 187, 255};
const Color PUFF_WHITE = (Color){241, 241, 241, 241};
const Color PUFF_BACKGROUND = (Color){6, 24, 24, 255};

Client* make_client(Pendulum* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    InitWindow(WIDTH, HEIGHT, "puffer Pendulum");
    SetTargetFPS(60);
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

void c_render(Pendulum* env) {
    if (IsKeyDown(KEY_ESCAPE))
        exit(0);
    if (IsKeyPressed(KEY_TAB))
        ToggleFullscreen();

    if (env->client == NULL) {
        env->client = make_client(env);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);
    DrawCircle(WIDTH / 2, HEIGHT / 2, 5, PUFF_CYAN);
    float pole_length = 2.0f * 0.5f * SCALE;
    float pole_x2 = (WIDTH / 2) + sinf(env->theta) * pole_length;
    float pole_y2 = (HEIGHT / 2) - cosf(env->theta) * pole_length;
    DrawLineEx((Vector2){WIDTH/2, HEIGHT / 2}, (Vector2){pole_x2, pole_y2}, 5, PUFF_RED);
    DrawText(TextFormat("Steps: %i", env->tick), 10, 10, 20, PUFF_WHITE);

    EndDrawing();
}

void compute_observations(Pendulum* env) {
    env->observations[0] = cosf(env->theta);
    env->observations[1] = sinf(env->theta);
    env->observations[2] = env->theta_dot;
}

float angle_normalize(Pendulum* env){
    float temp = fmod(env->theta + M_PI, 2 * M_PI);
    return (temp < 0 ? temp + 2 * M_PI : temp) - M_PI;
}

void c_reset(Pendulum* env) {
    env->episode_return = 0.0f;
    // env->x = ((float)rand() / (float)RAND_MAX) * 0.08f - 0.04f;
    // env->x_dot = ((float)rand() / (float)RAND_MAX) * 0.08f - 0.04f;
    env->theta = ((float)rand() / (float)RAND_MAX) * 2 * DEFAULT_THETA - DEFAULT_THETA;
    env->theta_dot = ((float)rand() / (float)RAND_MAX) * 2 * DEFAULT_THETA_DOT - DEFAULT_THETA_DOT;
    env->tick = 0;
    
    compute_observations(env);
}

void c_step(Pendulum* env) {  
    // float force = 0.0;
    // if (env->continuous) {
    //     force = env->actions[0] * FORCE_MAG;
    // } else {
    //     force = (env->actions[0] > 0.5f) ? FORCE_MAG : -FORCE_MAG; 
    // }

    float a = env->actions[0];

    /* ===== runtime sanity check –– delete after debugging ===== */
    if (!isfinite(a) || a < -2.0001f || a > 2.0001f) {
        fprintf(stderr,
                "[BAD ACTION] tick=%d  raw=%.6f\n",
                env->tick, a);
        fflush(stderr);
    }
    /* ========================================================== */

    if (!isfinite(a))             a = 0.0f;
    a = fminf(fmaxf(a, -MAX_TORQUE), MAX_TORQUE);
    env->actions[0] = a;

    float costs = angle_normalize(env) * angle_normalize(env) + 0.1 * env->theta_dot * env->theta_dot + 0.001 * a * a;
    // fprintf(stderr, "costs : %f\n", costs);
    // fprintf(stderr, "obs : %f %f %f\n", env->observations[0], env->observations[1], env->observations[2]);

    float new_theta_dot = env->theta_dot + (3.0f * GRAVITY / (2.0f * L) * sinf(env->theta) + 3.0f / (M * (L * L)) * a) * DT;
    new_theta_dot = fminf(fmaxf(new_theta_dot, -MAX_SPEED), MAX_SPEED);
    float new_theta = env->theta + new_theta_dot * DT;

    env->theta = new_theta;
    env->theta_dot = new_theta_dot;

    env->tick += 1;

    env->rewards[0] = -costs;
    env->episode_return += env->rewards[0];

    bool terminated = false;
    bool truncated = env->tick >= MAX_STEPS;
    bool done = terminated || truncated;

    env->terminals[0] = done;

    if (done) {
        add_log(env);
        c_reset(env);
    }

    compute_observations(env);
}
