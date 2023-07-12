#include <WindowContext/GLFWContext.hpp>

using namespace io;

template<typename ...Args>
struct ICallback {
    virtual void operator()(Args...) = 0;
};

template<typename ...Args>
struct FunctionCallback : ICallback<Args...>{
    std::function<void(Args...)> fn;
    void operator()(Args... args) { fn(args...); }
};


template<typename IFace, typename ...Args>
struct PtrCallback : ICallback<Args...>{
    IFace* ptr;
    void (IFace::*fn)(Args...);
    void operator()(Args... args) { (*ptr.*fn)(args...); }
};

template<typename ...Args>
struct CallbackUtils {
    using IFace_t = ICallback<Args...>;
    using IFaceUptr_t = std::unique_ptr<IFace_t>;

    template<typename IListener, typename FN>
    static IFaceUptr_t make(IListener* listener, FN fn) {
        auto cb = std::make_unique<PtrCallback<IListener, Args...>>();
        cb->ptr = listener;
        cb->fn = fn;
        return std::move(cb);
    }
    static IFaceUptr_t make(std::function<void(Args...)> fn) {
        auto cb = std::make_unique<FunctionCallback<Args...>>();
        cb->fn = fn;
        return std::move(cb);
    }
};
using IKeyInputCallbackIUtils_t = CallbackUtils<int, int, int>;
using ICursorPositionCallbackIUtils_t = CallbackUtils<double, double>;
using IMouseMovementCallbackIUtils_t = CallbackUtils<double, double>;
using IMouseInputCallbackIUtils_t = CallbackUtils<int, int, int>;
using IWindowResizeCallbackIUtils_t = CallbackUtils<int, int>;
using ICharacterInputCallbackUtils_t = CallbackUtils<uint32_t>;
using IScrollInputCallbackUtils_t = CallbackUtils<double, double>;

struct GLFWContext::Dispatcher {
    static IKeyInputCallbackIUtils_t::IFaceUptr_t key_input_callback;
    static ICursorPositionCallbackIUtils_t::IFaceUptr_t cursor_position_callback;
    static IMouseMovementCallbackIUtils_t::IFaceUptr_t mouse_movement_callback;
    static IMouseInputCallbackIUtils_t::IFaceUptr_t mouse_input_callback;
    static IWindowResizeCallbackIUtils_t::IFaceUptr_t window_resize_callback;
    static ICharacterInputCallbackUtils_t::IFaceUptr_t character_input_callback;
    static IScrollInputCallbackUtils_t::IFaceUptr_t scroll_input_callback;
    static std::atomic_bool cursor_mode;
    static std::atomic_bool active;
    static std::atomic_int center_w, center_h;

    static void set_center(int w, int h) {
        center_w = w/2;
        center_h = h/2;
    }
    static void set_center(const std::tuple<int, int>& v) {
        set_center(std::get<0>(v), std::get<1>(v));
    }

    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if(!active.load()) return;
        if(key_input_callback)
            (*key_input_callback)(key, action, mods);
    }
    static void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
        if(!active.load()) return;
        if(cursor_mode.load() && cursor_position_callback) {
            (*cursor_position_callback)(xpos, ypos);
        }
        else if(mouse_movement_callback) {
            (*mouse_movement_callback)(xpos - center_w, ypos - center_h);
            glfwSetCursorPos(window, center_w, center_h);
        }
    }
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        if(!active.load()) return;
        if(mouse_input_callback)
            (*mouse_input_callback)(button, action, mods);
    }
    static void window_size_callback(GLFWwindow* window, int width, int height) {
        set_center(width, height);
        if(!cursor_mode.load())
            glfwSetCursorPos(window, center_w, center_h);
        if(window_resize_callback)
            (*window_resize_callback)(width, height);
    }
    static void character_callback(GLFWwindow* window, uint32_t codepoint) {
        if(!active.load()) return;
        if(character_input_callback)
            (*character_input_callback)(codepoint);
    }
    static void scroll_callback(GLFWwindow* window, double xdelta, double ydelta) {
        if(!active.load()) return;
        if(scroll_input_callback)
            (*scroll_input_callback)(xdelta, ydelta);
    }
    static void focus_callback(GLFWwindow* window, int focused) {
        active = GLFW_TRUE == focused;
    }
};

IKeyInputCallbackIUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::key_input_callback{nullptr};
ICursorPositionCallbackIUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::cursor_position_callback{nullptr};
IMouseMovementCallbackIUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::mouse_movement_callback{nullptr};
IMouseInputCallbackIUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::mouse_input_callback{nullptr};
IWindowResizeCallbackIUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::window_resize_callback{nullptr};
ICharacterInputCallbackUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::character_input_callback{nullptr};
IScrollInputCallbackUtils_t::IFaceUptr_t
    GLFWContext::Dispatcher::scroll_input_callback{nullptr};

std::atomic_bool GLFWContext::Dispatcher::cursor_mode{false};
std::atomic_bool GLFWContext::Dispatcher::active{true};
std::atomic_int GLFWContext::Dispatcher::center_w, GLFWContext::Dispatcher::center_h;

void GLFWContext::input_listener_thread_fn() {
    glfwSetKeyCallback(window, Dispatcher::key_callback);
    glfwSetCursorPosCallback(window, Dispatcher::cursor_callback);
    glfwSetMouseButtonCallback(window, Dispatcher::mouse_button_callback);
    glfwSetWindowSizeCallback(window, Dispatcher::window_size_callback);
    glfwSetCharCallback(window, Dispatcher::character_callback);
    glfwSetScrollCallback(window, Dispatcher::scroll_callback);
    glfwSetWindowFocusCallback(window, Dispatcher::focus_callback);
    while(running) {
        glfwWaitEvents();
    }
}

GLFWContext::GLFWContext() {
    if (!glfwInit())
        throw std::runtime_error("Couldn't init glfw");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Couldn't create window");
    }
    glfwMakeContextCurrent(window);

    Dispatcher::set_center(get_dimensions());

    running = true;
    t = std::thread([this](){input_listener_thread_fn();});

    if (glfwRawMouseMotionSupported()) {
        // std::cout << "Raw mouse pos is supported" << std::endl;
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
        // std::cout << "Raw mouse pos is not supported" << std::endl;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

GLFWContext::~GLFWContext() {
    running = false;
    glfwPostEmptyEvent();
    t.join();
    if (window)
        glfwTerminate();
}

void GLFWContext::set_key_input_listener(IKeyInputListener* il) {
    Dispatcher::key_input_callback =
        IKeyInputCallbackIUtils_t::make(il, &IKeyInputListener::serve_key_input);
}
void GLFWContext::set_cursor_position_listener(ICursorPositionListener* pl) {
    Dispatcher::cursor_position_callback =
        ICursorPositionCallbackIUtils_t::make(pl, &ICursorPositionListener::serve_cursor_position);
}
void GLFWContext::set_mouse_movement_listener(IMouseMovementListener* ml) {
    Dispatcher::mouse_movement_callback =
        IMouseMovementCallbackIUtils_t::make(ml, &IMouseMovementListener::serve_mouse_movement);
}
void GLFWContext::set_mouse_input_listener(IMouseInputListener* ml) {
    Dispatcher::mouse_input_callback =
        IMouseInputCallbackIUtils_t::make(ml, &IMouseInputListener::serve_mouse_input);
}
void GLFWContext::set_window_resized_listener(IWindowResizeListener* rl) {
    Dispatcher::window_resize_callback =
        IWindowResizeCallbackIUtils_t::make(rl, &IWindowResizeListener::serve_window_resized);
}
void GLFWContext::set_character_listener(ICharacterInputListener* cl) {
    Dispatcher::character_input_callback =
        ICharacterInputCallbackUtils_t::make(cl, &ICharacterInputListener::serve_character);
}
void GLFWContext::set_scroll_input_listener(IScrollIuputListener *sl) {
    Dispatcher::scroll_input_callback =
        IScrollInputCallbackUtils_t::make(sl, &IScrollIuputListener::serve_scroll_input);
}

void GLFWContext::set_cursor_mode(bool val) {
    Dispatcher::cursor_mode = val;
    if(val) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        Dispatcher::cursor_callback(window, xpos, ypos);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported())
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}

void GLFWContext::set_sticky_keys(bool val) {
    glfwSetInputMode(window, GLFW_STICKY_KEYS, (val ? GLFW_TRUE : GLFW_FALSE));
}

bool GLFWContext::update() {
    glfwPollEvents();
    if(glfwWindowShouldClose(window))
        return false;

    glfwSwapBuffers(window);
    return true;
}

IWindowContext &GLFWContext::get() {
    static GLFWContext i;
    return i;
}

void GLFWContext::set_key_input_callback(std::function<void (int, int, int)> fn) {
    Dispatcher::key_input_callback = IKeyInputCallbackIUtils_t::make(fn);
}
void GLFWContext::set_cursor_position_callback(std::function<void (double, double)> fn) {
    Dispatcher::cursor_position_callback = ICursorPositionCallbackIUtils_t::make(fn);
}
void GLFWContext::set_mouse_movement_callback(std::function<void (double, double)> fn) {
    Dispatcher::mouse_movement_callback = IMouseMovementCallbackIUtils_t::make(fn);
}
void GLFWContext::set_mouse_input_callback(std::function<void (int, int, int)> fn) {
    Dispatcher::mouse_input_callback = IMouseInputCallbackIUtils_t::make(fn);
}
void GLFWContext::set_window_resized_callback(std::function<void (int, int)> fn) {
    Dispatcher::window_resize_callback = IWindowResizeCallbackIUtils_t::make(fn);
}
void GLFWContext::set_character_callback(std::function<void (uint32_t)> fn) {
    Dispatcher::character_input_callback = ICharacterInputCallbackUtils_t::make(fn);
}
void GLFWContext::set_scroll_input_callback(std::function<void (double, double)> fn){
    Dispatcher::scroll_input_callback = IScrollInputCallbackUtils_t::make(fn);
}

std::tuple<int, int> GLFWContext::get_dimensions() {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    return {w, h};
}
