#include "squared2.h"
#include "puffernet.h"

void demo(){
    //Weights* weights = load_weights("resources/pong_weights.bin", 133764);
    //LinearLSTM* net = make_linearlstm(weights, 1, 8, 3);

    Squared2 env = { .grid_size = 5};
    allocate(&env);
    // init(&env);
    // generate_board_positions(&env);

    Client* client = make_client(&env);
    reset(&env);
    int action_idx;
    while (!WindowShouldClose()) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            env.actions[0] = 0;
            if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) env.actions[0] = UP;
            if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) env.actions[0] = DOWN;
            if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) env.actions[0] = LEFT;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) env.actions[0] = RIGHT;
        } else {
            env.actions[0] = NOOP;
            //forward_linearlstm(net, env.observations, env.actions);
        }
        action_idx = rand() % env.num_empty_tiles;
        env.actions[0] = env.possible_moves[action_idx];
        step(&env);
        render(client, &env);
    }
    //free_linearlstm(net);
    //free(weights);
    free_allocated(&env);
    close_client(client);
}

void test_performance(float test_time) {
    Squared2 env = { .grid_size = 5};
    allocate(&env);
    reset(&env);

    int start = time(NULL);
    int i = 0;
    int action_idx;
    while (time(NULL) - start < test_time) {
        int action_idx = rand() % env.num_empty_tiles;
        env.actions[0] = env.possible_moves[action_idx];
        step(&env);
        i++;
    }
    int end = time(NULL);
    printf("SPS: %f\n", (float)i / (end - start));
    free_allocated(&env);
}

int main() {
    // demo();
    test_performance(5);
    return 0;
}
