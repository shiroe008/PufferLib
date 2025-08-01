#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

#define GRAVITY 9.8f
#define DT 0.2f

#define LINK_LENGTH_1 1.0f
#define LINK_LENGTH_2 1.0f
#define LINK_MASS_1 1.0f
#define LINK_MASS_2 1.0f
#define LINK_COM_POS_1 0.5f
#define LINK_COM_POS_2 0.5f
#define LINK_MOI 1.0f

#define MAX_VEL_1 (4.0f * M_PI)
#define MAX_VEL_2 (9.0f * M_PI)

#define MAX_STEPS 500
#define WIDTH 500
#define HEIGHT 500

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

typedef struct Acrobot Acrobot;
struct Acrobot {
    float* observations;
    float* actions;
    float* rewards;
    unsigned char* terminals;
    unsigned char* truncations;
    Log log;
    Client* client;
    int tick;
    float episode_return;
    
    // State variables: [theta1, theta2, dtheta1, dtheta2]
    float theta1;
    float theta2;
    float dtheta1;
    float dtheta2;
    
    bool book_or_nips;  // true for book, false for nips
};

void add_log(Acrobot* env) {
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

void init(Acrobot* env) {
    env->tick = 0;
    env->book_or_nips = true;  // default to book dynamics
    memset(&env->log, 0, sizeof(Log));
}

void allocate(Acrobot* env) {
    init(env);
    env->observations = (float*)calloc(6, sizeof(float));
    env->actions = (float*)calloc(1, sizeof(float));
    env->rewards = (float*)calloc(1, sizeof(float));
    env->terminals = (unsigned char*)calloc(1, sizeof(unsigned char));
}

void free_allocated(Acrobot* env) {
    free(env->observations);
    free(env->actions);
    free(env->rewards);
    free(env->terminals);
}

void c_close(Acrobot* env) {
}

const Color PUFF_RED = (Color){187, 0, 0, 255};
const Color PUFF_CYAN = (Color){0, 187, 187, 255};
const Color PUFF_WHITE = (Color){241, 241, 241, 241};
const Color PUFF_BACKGROUND = (Color){6, 24, 24, 255};
const Color PUFF_GREEN = (Color){0, 204, 204, 255};
const Color PUFF_YELLOW = (Color){204, 204, 0, 255};

Client* make_client(Acrobot* env) {
    Client* client = (Client*)calloc(1, sizeof(Client));
    InitWindow(WIDTH, HEIGHT, "puffer Acrobot");
    SetTargetFPS(30);
    return client;
}

void close_client(Client* client) {
    CloseWindow();
    free(client);
}

float wrap(float x, float m, float M) {
    float diff = M - m;
    while (x > M) {
        x = x - diff;
    }
    while (x < m) {
        x = x + diff;
    }
    return x;
}

float bound(float x, float m, float M) {
    return fminf(fmaxf(x, m), M);
}

void c_render(Acrobot* env) {
    if (IsKeyDown(KEY_ESCAPE))
        exit(0);
    if (IsKeyPressed(KEY_TAB))
        ToggleFullscreen();

    if (env->client == NULL) {
        env->client = make_client(env);
    }

    BeginDrawing();
    ClearBackground(PUFF_BACKGROUND);
    
    float bound_val = LINK_LENGTH_1 + LINK_LENGTH_2 + 0.2f;
    float scale = (float)HEIGHT / (bound_val * 2);
    float offset_x = WIDTH / 2;
    float offset_y = HEIGHT / 2;
    
    // Draw target line (above the system)
    DrawLine(0, offset_y - 1.0f * scale,
             WIDTH, offset_y - 1.0f * scale, PUFF_WHITE);
    
    // Calculate link positions (note: y is inverted in screen coordinates)
    float p1_x = LINK_LENGTH_1 * sinf(env->theta1) * scale;
    float p1_y = LINK_LENGTH_1 * cosf(env->theta1) * scale;
    
    float p2_x = p1_x + LINK_LENGTH_2 * sinf(env->theta1 + env->theta2) * scale;
    float p2_y = p1_y + LINK_LENGTH_2 * cosf(env->theta1 + env->theta2) * scale;
    
    // Draw first link
    DrawLineEx((Vector2){offset_x, offset_y}, 
               (Vector2){offset_x + p1_x, offset_y + p1_y}, 5, PUFF_GREEN);
    
    // Draw second link
    DrawLineEx((Vector2){offset_x + p1_x, offset_y + p1_y}, 
               (Vector2){offset_x + p2_x, offset_y + p2_y}, 5, PUFF_GREEN);
    
    // Draw joints
    DrawCircle(offset_x, offset_y, 0.1f * scale, PUFF_YELLOW);
    DrawCircle(offset_x + p1_x, offset_y + p1_y, 0.1f * scale, PUFF_YELLOW);
    DrawCircle(offset_x + p2_x, offset_y + p2_y, 0.1f * scale, PUFF_YELLOW);
    
    DrawText(TextFormat("Steps: %i", env->tick), 10, 10, 20, PUFF_WHITE);
    
    EndDrawing();
}

void compute_observations(Acrobot* env) {
    env->observations[0] = cosf(env->theta1);
    env->observations[1] = sinf(env->theta1);
    env->observations[2] = cosf(env->theta2);
    env->observations[3] = sinf(env->theta2);
    env->observations[4] = env->dtheta1;
    env->observations[5] = env->dtheta2;
}

bool is_terminal(Acrobot* env) {
    return (-cosf(env->theta1) - cosf(env->theta2 + env->theta1) > 1.0f);
}


void dsdt(Acrobot* env, float torque, float* derivatives) {
    float m1 = LINK_MASS_1;
    float m2 = LINK_MASS_2;
    float l1 = LINK_LENGTH_1;
    float lc1 = LINK_COM_POS_1;
    float lc2 = LINK_COM_POS_2;
    float I1 = LINK_MOI;
    float I2 = LINK_MOI;
    float g = GRAVITY;
    
    float theta1 = env->theta1;
    float theta2 = env->theta2;
    float dtheta1 = env->dtheta1;
    float dtheta2 = env->dtheta2;
    
    float d1 = m1 * lc1 * lc1 + m2 * (l1 * l1 + lc2 * lc2 + 2 * l1 * lc2 * cosf(theta2)) + I1 + I2;
    float d2 = m2 * (lc2 * lc2 + l1 * lc2 * cosf(theta2)) + I2;
    float phi2 = m2 * lc2 * g * cosf(theta1 + theta2 - M_PI / 2.0f);
    float phi1 = -m2 * l1 * lc2 * dtheta2 * dtheta2 * sinf(theta2)
                 - 2 * m2 * l1 * lc2 * dtheta2 * dtheta1 * sinf(theta2)
                 + (m1 * lc1 + m2 * l1) * g * cosf(theta1 - M_PI / 2.0f)
                 + phi2;
    
    float ddtheta2;
    if (!env->book_or_nips) {
        // NIPS paper dynamics
        ddtheta2 = (torque + d2 / d1 * phi1 - phi2) / (m2 * lc2 * lc2 + I2 - d2 * d2 / d1);
    } else {
        // Book dynamics (default)
        ddtheta2 = (torque + d2 / d1 * phi1 - m2 * l1 * lc2 * dtheta1 * dtheta1 * sinf(theta2) - phi2) 
                   / (m2 * lc2 * lc2 + I2 - d2 * d2 / d1);
    }
    
    float ddtheta1 = -(d2 * ddtheta2 + phi1) / d1;
    
    derivatives[0] = dtheta1;
    derivatives[1] = dtheta2;
    derivatives[2] = ddtheta1;
    derivatives[3] = ddtheta2;
}

void rk4_step(Acrobot* env, float torque) {
    float dt = DT;
    float dt2 = dt / 2.0f;
    
    float k1[4], k2[4], k3[4], k4[4];
    float temp_state[4];
    
    // k1
    dsdt(env, torque, k1);
    
    // k2
    temp_state[0] = env->theta1 + dt2 * k1[0];
    temp_state[1] = env->theta2 + dt2 * k1[1];
    temp_state[2] = env->dtheta1 + dt2 * k1[2];
    temp_state[3] = env->dtheta2 + dt2 * k1[3];
    
    float orig_theta1 = env->theta1;
    float orig_theta2 = env->theta2;
    float orig_dtheta1 = env->dtheta1;
    float orig_dtheta2 = env->dtheta2;
    
    env->theta1 = temp_state[0];
    env->theta2 = temp_state[1];
    env->dtheta1 = temp_state[2];
    env->dtheta2 = temp_state[3];
    dsdt(env, torque, k2);
    
    // k3
    env->theta1 = orig_theta1 + dt2 * k2[0];
    env->theta2 = orig_theta2 + dt2 * k2[1];
    env->dtheta1 = orig_dtheta1 + dt2 * k2[2];
    env->dtheta2 = orig_dtheta2 + dt2 * k2[3];
    dsdt(env, torque, k3);
    
    // k4
    env->theta1 = orig_theta1 + dt * k3[0];
    env->theta2 = orig_theta2 + dt * k3[1];
    env->dtheta1 = orig_dtheta1 + dt * k3[2];
    env->dtheta2 = orig_dtheta2 + dt * k3[3];
    dsdt(env, torque, k4);
    
    // Update state
    env->theta1 = orig_theta1 + dt / 6.0f * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]);
    env->theta2 = orig_theta2 + dt / 6.0f * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]);
    env->dtheta1 = orig_dtheta1 + dt / 6.0f * (k1[2] + 2 * k2[2] + 2 * k3[2] + k4[2]);
    env->dtheta2 = orig_dtheta2 + dt / 6.0f * (k1[3] + 2 * k2[3] + 2 * k3[3] + k4[3]);
}

void c_reset(Acrobot* env) {
    env->episode_return = 0.0f;
    
    // Initialize state uniformly between -0.1 and 0.1
    env->theta1 = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    env->theta2 = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    env->dtheta1 = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    env->dtheta2 = ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
    
    env->tick = 0;
    
    compute_observations(env);
}

void c_step(Acrobot* env) {
    // Convert discrete action to torque
    int action = (int)env->actions[0];
    float torque = action - 1.0f;
    
    // Integrate dynamics using RK4
    rk4_step(env, torque);
    
    // Wrap and bound the state
    env->theta1 = wrap(env->theta1, -M_PI, M_PI);
    env->theta2 = wrap(env->theta2, -M_PI, M_PI);
    env->dtheta1 = bound(env->dtheta1, -MAX_VEL_1, MAX_VEL_1);
    env->dtheta2 = bound(env->dtheta2, -MAX_VEL_2, MAX_VEL_2);
    
    env->tick += 1;
    
    bool terminated = is_terminal(env);
    env->rewards[0] = terminated ? 0.0f : -1.0f;
    env->episode_return += env->rewards[0];
    
    bool truncated = env->tick >= MAX_STEPS;
    bool done = terminated || truncated;
    
    env->terminals[0] = done;
    
    if (done) {
        add_log(env);
        c_reset(env);
    }
    
    compute_observations(env);
}
