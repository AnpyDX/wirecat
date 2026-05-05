#include "application.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>

#define WIRECAT_GLOBAL_FONT "fonts/consolab.ttf"

namespace WireCat {
    Application::Application(const std::string& title, int width, int height) {
        /* Windows creation and GL context initialization. */
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (window == nullptr) {
            throw std::runtime_error("failed to create GLFW window");
        }
        
        glfwMakeContextCurrent(window);
        int gladResult = gladLoadGL(glfwGetProcAddress);
        if (gladResult == 0) {
            throw std::runtime_error("failed to initialize OpenGL context. Please make sure system supports OpenGL 3.3+");
        }

        glfwSwapInterval(GLFW_TRUE);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
            glViewport(0, 0, w, h);
        });

        /* ImGui context initialization. */
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr; // Disable imgui.ini
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        const int defaultFontSize = 14.0;
        ImGui::GetIO().Fonts->AddFontFromFileTTF(
            WIRECAT_GLOBAL_FONT, defaultFontSize, nullptr,
            ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon()
        );

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        float windowMainScale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
        style.ScaleAllSizes(windowMainScale);
        style.FontScaleDpi = windowMainScale;
        style.WindowBorderSize = 2;
        style.WindowRounding = 1.0;
        style.ChildRounding = 1.0;
        style.FrameRounding = 1.0;
        style.PopupRounding = 1.0;
        style.GrabRounding = 1.0;
        style.TabRounding = 3;
        style.ScrollbarSize = 15;
    }

    Application::~Application() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (window != nullptr) {
            glfwDestroyWindow(window);
            glfwTerminate();
            window = nullptr;
        }
    }

    void Application::launch() {
        while (glfwWindowShouldClose(window) != GLFW_TRUE) {
            glClearColor(0.1, 0.1, 0.1, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            renderUI();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
}