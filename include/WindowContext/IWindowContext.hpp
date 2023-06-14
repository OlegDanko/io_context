#pragma once
#include <cinttypes>
#include <tuple>

namespace io {

class IKeyInputListener {
public:
    virtual void serve_key_input(int key, int action, int mods) = 0;
    ~IKeyInputListener() = default;
};

class ICursorPositionListener {
public:
    virtual void serve_cursor_position(double xpos, double ypos) = 0;
    ~ICursorPositionListener() = default;
};

class IMouseMovementListener {
public:
    virtual void serve_mouse_movement(double xpos, double ypos) = 0;
    ~IMouseMovementListener() = default;
};

class IMouseInputListener {
public:
    virtual void serve_mouse_input(int button, int action, int mods) = 0;
    ~IMouseInputListener() = default;
};

class IWindowResizeListener {
public:
    virtual void serve_window_resized(int width, int height) = 0;
    ~IWindowResizeListener() = default;
};

class ICharacterListener {
public:
    virtual void serve_character(uint32_t codepoint) = 0;
    ~ICharacterListener() = default;
};

class IWindowContext {
public:
    virtual void set_listener(IKeyInputListener* il) = 0;
    virtual void set_listener(ICursorPositionListener* pl) = 0;
    virtual void set_listener(IMouseMovementListener* ml) = 0;
    virtual void set_listener(IMouseInputListener* il) = 0;
    virtual void set_listener(IWindowResizeListener* il) = 0;
    virtual void set_listener(ICharacterListener* cl) = 0;

    virtual void set_sticky_keys(bool val) = 0;
    virtual void set_cursor_mode(bool val) = 0;
    virtual bool update() = 0;
    virtual std::tuple<int, int> get_dimensions() = 0;
    ~IWindowContext() = default;
};

}
