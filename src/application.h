#pragma once
#include <string>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace WireCat {
    class Application {
    public:
        Application(const std::string& title, int width, int height);

        ~Application();

        ///! Launch app's main loop.
        void launch();

        ///! UI rendering in every render loop.
        virtual void renderUI() = 0;

    protected:
        GLFWwindow* window = nullptr;
    };
}