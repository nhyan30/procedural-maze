// Window.hpp - RAII GLFW window + input snapshot.
//
// GLFW callbacks write into an InputState that the game reads once per frame
// and then clears. This keeps event handling out of gameplay code: systems
// only ever see a plain struct.
#pragma once

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace maze {

struct InputState {
    static constexpr int kKeyCount = GLFW_KEY_LAST + 1;

    bool down[kKeyCount] = {};    // held during this frame
    bool pressed[kKeyCount] = {}; // went down this frame (edge-triggered)
    float mouseDX = 0.0f;         // accumulated cursor movement since last frame
    float mouseDY = 0.0f;

    bool key(int k) const { return k >= 0 && k < kKeyCount && down[k]; }
    bool keyPressed(int k) const { return k >= 0 && k < kKeyCount && pressed[k]; }
    void endFrame();
};

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void requestClose();
    void pollEvents();
    void swapBuffers();

    int framebufferWidth() const { return fbWidth_; }
    int framebufferHeight() const { return fbHeight_; }
    float aspect() const {
        return fbHeight_ > 0
                   ? static_cast<float>(fbWidth_) / static_cast<float>(fbHeight_)
                   : 1.0f;
    }

    InputState& input() { return input_; }
    GLFWwindow* handle() const { return window_; }

private:
    static void errorCallback(int code, const char* description);
    static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void framebufferSizeCallback(GLFWwindow* w, int width, int height);

    GLFWwindow* window_ = nullptr;
    InputState input_;
    int fbWidth_ = 0;
    int fbHeight_ = 0;
    double lastCursorX_ = 0.0;
    double lastCursorY_ = 0.0;
    bool hasLastCursor_ = false;
};

} // namespace maze
