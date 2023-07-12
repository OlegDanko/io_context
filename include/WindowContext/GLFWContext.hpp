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
    static IWindowContext& get();

    void set_key_input_listener(IKeyInputListener* il) override;
    void set_cursor_position_listener(ICursorPositionListener* pl) override;
    void set_mouse_movement_listener(IMouseMovementListener* ml) override;
    void set_mouse_input_listener(IMouseInputListener* il) override;
    void set_window_resized_listener(IWindowResizeListener* il) override;
    void set_character_listener(ICharacterInputListener* cl) override;
    void set_scroll_input_listener(IScrollIuputListener *sl) override;

    void set_key_input_callback(std::function<void(int, int, int)>) override;
    void set_cursor_position_callback(std::function<void(double, double)>) override;
    void set_mouse_movement_callback(std::function<void(double, double)>) override;
    void set_mouse_input_callback(std::function<void(int, int, int)>) override;
    void set_window_resized_callback(std::function<void(int, int)>) override;
    void set_character_callback(std::function<void(uint32_t)>) override;
    void set_scroll_input_callback(std::function<void (double, double)>) override;

    void set_cursor_mode(bool val) override;
    void set_sticky_keys(bool val) override;
    bool update() override;
    std::tuple<int, int> get_dimensions() override;
};

}
