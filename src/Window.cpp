#include "Window.hpp"

#include <cstdio>
#include <stdexcept>

namespace maze {

void InputState::endFrame() {
    for (bool& p : pressed) p = false;
    mouseDX = 0.0f;
    mouseDY = 0.0f;
}

void Window::errorCallback(int code, const char* description) {
    std::fprintf(stderr, "[GLFW %d] %s\n", code, description);
}

void Window::keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action,
                         int /*mods*/) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self || key < 0 || key >= InputState::kKeyCount) return;
    if (action == GLFW_PRESS) {
        self->input_.down[key] = true;
        self->input_.pressed[key] = true;
    } else if (action == GLFW_RELEASE) {
        self->input_.down[key] = false;
    }
    // GLFW_REPEAT is ignored: held keys stay marked in `down`.
}

void Window::cursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    if (!self->hasLastCursor_) {
        self->lastCursorX_ = x;
        self->lastCursorY_ = y;
        self->hasLastCursor_ = true;
        return;
    }
    self->input_.mouseDX += static_cast<float>(x - self->lastCursorX_);
    self->input_.mouseDY += static_cast<float>(y - self->lastCursorY_);
    self->lastCursorX_ = x;
    self->lastCursorY_ = y;
}

void Window::framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
    if (!self) return;
    self->fbWidth_ = width;
    self->fbHeight_ = height;
}

Window::Window(int width, int height, const char* title) {
    glfwSetErrorCallback(errorCallback);

    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("Window: glfwInit failed");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Required by macOS to get a 3.3+ core context; ignored elsewhere.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA; llvmpipe and most GPUs support 4x

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Window: failed to create OpenGL 3.3 context");
    }

    // Centre the window on the primary monitor.
    if (const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor())) {
        glfwSetWindowPos(window_, (mode->width - width) / 2,
                         (mode->height - height) / 2);
    }

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, keyCallback);
    glfwSetCursorPosCallback(window_, cursorPosCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    // Load all GL entry points for this context and verify the feature level.
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0)
        throw std::runtime_error("Window: failed to load OpenGL entry points");
    if (!GLAD_GL_VERSION_3_3)
        throw std::runtime_error("Window: OpenGL 3.3 core is not available");
    std::printf("[gl] %s | %s\n",
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
                reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    // Mouse-look: hide the OS cursor and prefer raw (unaccelerated) motion.
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    glfwGetFramebufferSize(window_, &fbWidth_, &fbHeight_);
    glfwShowWindow(window_);
}

Window::~Window() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_) != 0; }

void Window::requestClose() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

void Window::pollEvents() { glfwPollEvents(); }

void Window::swapBuffers() { glfwSwapBuffers(window_); }

} // namespace maze
