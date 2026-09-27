#pragma once
#include "custom_types.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>

class Window
{
  private:
    GLFWwindow *window;

  public:
    Window(u32 width, u32 height)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, "Vulkan Tutorial", nullptr, nullptr);
    }

    ~Window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    GLFWwindow *get_glfw_window()
    {
        return window;
    }
};

void *open_file(const char *file_name, i32 *file_size);

void close_file(void *file);

struct Input
{
    bool w;
    bool a;
    bool s;
    bool d;
    bool left;
    bool right;
    bool up;
    bool down;
};
