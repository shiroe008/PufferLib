#include "hex.h"
#include "puffernet.h"

void demo(){
    //Weights* weights = load_weights("resources/pong_weights.bin", 133764);
    //LinearLSTM* net = make_linearlstm(weights, 1, 8, 3);

    Hex env = { .grid_size = 5};
    allocate(&env);

    Client* client = make_client(&env);
    reset(&env);
    int action_idx;
    while (!WindowShouldClose()) {
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
    Hex env = { .grid_size = 5};
    allocate(&env);
    reset(&env);

    int start = time(NULL);
    int i = 0;
    int action_idx;
    while (time(NULL) - start < test_time) {
        action_idx = rand() % env.num_empty_tiles;
        env.actions[0] = env.possible_moves[action_idx];
        step(&env);
        i++;
    }
    int end = time(NULL);
    printf("SPS: %f\n", (float)i / (end - start));
    free_allocated(&env);
}

int main() {
    //demo();
    test_performance(5);
    return 0;
}
