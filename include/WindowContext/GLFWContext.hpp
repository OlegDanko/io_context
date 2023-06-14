#pragma once

#include "IWindowContext.hpp"
#include <GLFW/glfw3.h>
#include <atomic>
#include <thread>
#include <stdexcept>

namespace io {

class GLFWContext : public IWindowContext {
    GLFWwindow* window{nullptr};
    std::thread t;
    std::atomic_bool running{false};
    struct Dispatcher;

    void input_listener_thread_fn();
    GLFWContext();
    ~GLFWContext();
    GLFWContext(const GLFWContext&) = delete;
    GLFWContext operator=(const GLFWContext&) = delete;
    std::tuple<int, int> get_dimensions_();
public:
    static IWindowContext& get() {
        static GLFWContext i;
        return i;
    }

    void set_listener(IKeyInputListener* il) override;
    void set_listener(ICursorPositionListener* pl) override;
    void set_listener(IMouseMovementListener* ml) override;
    void set_listener(IMouseInputListener* il) override;
    void set_listener(IWindowResizeListener* il) override;
    void set_listener(ICharacterListener* cl) override;

    void set_cursor_mode(bool val) override;
    void set_sticky_keys(bool val) override;
    bool update() override;
    std::tuple<int, int> get_dimensions() override;
};

}
