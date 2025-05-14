#include "cel.h"
#include "cel_log.h"
#include "cel_vulkan.h"

#include <GLFW/glfw3.h>

typedef struct CELapp_state CELapp_state;
struct CELapp_state {
    CELgame *game_inst;
    GLFWwindow *window;

    uint32_t actual_width;
    uint32_t actual_height;
    const char *title;
};

GlobalVariable bool is_initialized = false;
GlobalVariable CELapp_state state  = {0};

Internal void error_callback(int error, const char *description);
Internal void window_close_callback(GLFWwindow *window);
Internal void window_size_callback(GLFWwindow *window, int width, int height);

bool application_init(CELgame *game) {
    state.game_inst = game;

    state.actual_width  = game->config.width;
    state.actual_height = game->config.height;
    state.title         = game->config.title;

    if (!glfwInit()) { return false; }

    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    state.window = glfwCreateWindow((int) state.actual_width, (int) state.actual_height, state.title, NULL, NULL);
    if (!state.window) { return false; }

    glfwSetWindowCloseCallback(state.window, window_close_callback);
    glfwSetWindowSizeCallback(state.window, window_size_callback);

    // setup vulkan
    if (!cel_vulkan_init(state.window)) { return false; };

    if (!state.game_inst->game_init(state.game_inst)) { return false; }

    is_initialized = true;

    CEL_INFO("application initialize successfully!");
    return true;
}

bool application_run() {
    while (!glfwWindowShouldClose(state.window))
    {
        glfwPollEvents();

        if (!state.game_inst->game_draw(state.game_inst)) { return false; }
    }

    cel_vulkan_fini();
    return true;
}

bool application_resize() {
    printf("app resize");
    return true;
}

void error_callback(int error, const char *description) {
    printf("glfw error (code %d): %s", error, description);
}

void window_close_callback(GLFWwindow *window) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void window_size_callback(GLFWwindow *window, int width, int height) {
    application_resize();
}
