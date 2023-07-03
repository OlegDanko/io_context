#include <WindowContext/GLFWContext.hpp>

using namespace io;

struct GLFWContext::Dispatcher {
    static std::atomic<IKeyInputListener*> key_input_listener;
    static std::atomic<ICursorPositionListener*> cursor_position_listener;
    static std::atomic<IMouseMovementListener*> mouse_movement_listener;
    static std::atomic<IMouseInputListener*> mouse_input_listener;
    static std::atomic<IWindowResizeListener*> window_resize_listener;
    static std::atomic<ICharacterListener*> character_listener;
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
        auto il = key_input_listener.load();
        if(il) il->serve_key_input(key, action, mods);
    }
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        if(!active.load()) return;
        if(cursor_mode.load()) {
            auto pl = cursor_position_listener.load();
            if(pl) pl->serve_cursor_position(xpos, ypos);
        }
        else {
            auto ml = mouse_movement_listener.load();
            if(ml) ml->serve_mouse_movement(xpos - center_w, ypos - center_h);
            glfwSetCursorPos(window, center_w, center_h);
        }
    }
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        if(!active.load()) return;
        auto ml = mouse_input_listener.load();
        if(ml) ml->serve_mouse_input(button, action, mods);
    }
    static void window_size_callback(GLFWwindow* window, int width, int height) {
        set_center(width, height);
        if(!cursor_mode.load())
            glfwSetCursorPos(window, center_w, center_h);
        auto wl = window_resize_listener.load();
        if(wl) wl->serve_window_resized(width, height);
    }
    static void character_callback(GLFWwindow* window, uint32_t codepoint) {
        if(!active.load()) return;
        auto cl = character_listener.load();
        if(cl) cl->serve_character(codepoint);
    }
    static void focus_callback(GLFWwindow* window, int focused) {
        active = GLFW_TRUE == focused;
    }
};

std::atomic<IKeyInputListener*> GLFWContext::Dispatcher::key_input_listener{nullptr};
std::atomic<ICursorPositionListener*> GLFWContext::Dispatcher::cursor_position_listener{nullptr};
std::atomic<IMouseMovementListener*> GLFWContext::Dispatcher::mouse_movement_listener{nullptr};
std::atomic<IMouseInputListener*> GLFWContext::Dispatcher::mouse_input_listener{nullptr};
std::atomic<IWindowResizeListener*> GLFWContext::Dispatcher::window_resize_listener{nullptr};
std::atomic<ICharacterListener*> GLFWContext::Dispatcher::character_listener{nullptr};
std::atomic_bool GLFWContext::Dispatcher::cursor_mode{false};
std::atomic_bool GLFWContext::Dispatcher::active{true};
std::atomic_int GLFWContext::Dispatcher::center_w, GLFWContext::Dispatcher::center_h;

void GLFWContext::input_listener_thread_fn() {
    glfwSetKeyCallback(window, Dispatcher::key_callback);
    glfwSetCursorPosCallback(window, Dispatcher::cursor_position_callback);
    glfwSetMouseButtonCallback(window, Dispatcher::mouse_button_callback);
    glfwSetWindowSizeCallback(window, Dispatcher::window_size_callback);
    glfwSetCharCallback(window, Dispatcher::character_callback);
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

    Dispatcher::set_center(get_dimensions_());

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

void GLFWContext::set_listener(IKeyInputListener* il) {
    Dispatcher::key_input_listener = il;
}
void GLFWContext::set_listener(ICursorPositionListener* pl) {
    Dispatcher::cursor_position_listener = pl;
}
void GLFWContext::set_listener(IMouseMovementListener* ml) {
    Dispatcher::mouse_movement_listener = ml;
}
void GLFWContext::set_listener(IMouseInputListener* il) {
    Dispatcher::mouse_input_listener = il;
}
void GLFWContext::set_listener(IWindowResizeListener* il) {
    Dispatcher::window_resize_listener = il;
}
void GLFWContext::set_listener(ICharacterListener* cl) {
    Dispatcher::character_listener = cl;
}

void GLFWContext::set_cursor_mode(bool val) {
    Dispatcher::cursor_mode = val;
    if(val) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        Dispatcher::cursor_position_callback(window, xpos, ypos);
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
//    glfwPollEvents();
    if(glfwWindowShouldClose(window))
        return false;

    glfwSwapBuffers(window);
    return true;
}

std::tuple<int, int> GLFWContext::get_dimensions_() {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    return {w, h};
}

IWindowContext &GLFWContext::get() {
    static GLFWContext i;
    return i;
}

std::tuple<int, int> GLFWContext::get_dimensions() {
    return get_dimensions_();
}
