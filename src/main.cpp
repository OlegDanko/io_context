#include <iostream>
#include <list>
#include <unordered_map>
#include <functional>

#include <GLFW/glfw3.h>
#include <unicode/unistr.h>

#include "../include/WindowContext/GLFWContext.hpp"
using namespace io;

class KeyInputListenerBase : public IKeyInputListener {
    IKeyInputListener* next{nullptr};
    virtual bool _serve(int key, int action, int mods) = 0;
public:
    void serve_key_input(int key, int action, int mods) override {
        if(!_serve(key, action, mods) && next)
            next->serve_key_input(key, action, mods);
    }
    void set_next(IKeyInputListener* il) {
        next = il;
    }
};

class TextEdit : public ICharacterListener {
    std::list<uint32_t> characters;
    std::list<uint32_t>::iterator cursor{characters.end()};
    std::atomic_bool active{false};

    void print() {
        using It = std::list<uint32_t>::iterator;
        auto get_str = [](It begin, It end) {
            icu::UnicodeString uni_str;
            std::for_each(begin, end, [&uni_str](auto c){ uni_str.append((UChar32)c); });

            std::string str;
            uni_str.toUTF8String(str);
            return str;
        };

        std::cout << get_str(characters.begin(), cursor);

        if(cursor == characters.end()) {
            std::cout << "\u001b[7m \u001b[0m" << std::endl;
            return;
        }
        std::cout << "\u001b[7m" << get_str(cursor, std::next(cursor)) << "\u001b[0m";
        std::cout << get_str(std::next(cursor), characters.end()) << std::endl;
    }
public:
    TextEdit() {}

    void insert(uint32_t codepoint) {
        characters.insert(cursor, codepoint);

        print();
    }
public:
    bool is_active() {
        return active;
    }

    void set_active(bool val) {
        active = val;
    }

    void delete_() {
        if(cursor == characters.end()) return;
        cursor = characters.erase(cursor);

        print();
    }
    void backspace() {
        if(cursor == characters.begin()) return;
        --cursor;
        delete_();
    }
    void move_left() {
        if(cursor == characters.begin()) return;
        --cursor;

        print();
    }
    void move_right() {
        if(cursor == characters.end()) return;
        ++cursor;

        print();
    }

    void serve_character(uint32_t codepoint) override {
        if(is_active())
            insert(codepoint);
    }
};

class KeyInputListenerUi : public KeyInputListenerBase {
    TextEdit& te;

    bool _serve(int key, int action, int mods) override {
        if(te.is_active()) {
            if(action != GLFW_PRESS && action != GLFW_REPEAT)
                return true;
            switch(key) {
            case GLFW_KEY_ESCAPE:
                set_text_mode(false);
                break;
            case GLFW_KEY_BACKSPACE:
                te.backspace();
                break;
            case GLFW_KEY_DELETE:
                te.delete_();
                break;
            case GLFW_KEY_LEFT:
                te.move_left();
                break;
            case GLFW_KEY_RIGHT:
                te.move_right();
                break;
            }
            return true;
        }
        if(key == GLFW_KEY_J) {
           set_text_mode(true);
            return true;
        }
        return false;
    }
public:
    KeyInputListenerUi(TextEdit& te) : te(te) {}
    void set_text_mode(bool val) {
        io::GLFWContext::get().set_sticky_keys(!val);
        te.set_active(val);
        val ? std::cout << "entering text mode" << std::endl
            : std::cout << "exiting text mode" << std::endl;
    }
};

class KeyInputListenerGameObject : public KeyInputListenerBase {
public:
    std::unordered_map<size_t, std::function<void()>> callbacks;

    size_t hash(int key, int action = GLFW_PRESS, int mods = 0) {
        size_t h = key;
        h <<= 16;
        h |= action;
        h <<= 16;
        h |= mods;
        return h;
    }

    KeyInputListenerGameObject() {
        callbacks[hash(GLFW_KEY_W)] = [](){std::cout << "Forward" << std::endl;};
        callbacks[hash(GLFW_KEY_S)] = [](){std::cout << "Backward" << std::endl;};
        callbacks[hash(GLFW_KEY_A)] = [](){std::cout << "Left" << std::endl;};
        callbacks[hash(GLFW_KEY_D)] = [](){std::cout << "Right" << std::endl;};
    }

    void add_callback(int key, int action, int mods, std::function<void()> fn) {
        callbacks[hash(key, action, mods)] = std::move(fn);
    }

    bool _serve(int key, int action, int mods) override {
        if(auto it = callbacks.find(hash(key, action, mods)); it != callbacks.end()) {
            it->second();
            return true;
        }
        return false;
    }
};

class CursorPositionListener : public ICursorPositionListener {
public:
    void serve_cursor_position(double xpos, double ypos) override {
        std::cout << xpos << "; " << ypos << " - cursor" << std::endl;
    }
};

class MouseInputListener : public IMouseMovementListener {
public:
    void serve_mouse_movement(double xpos, double ypos) override {
        std::cout << xpos << "; " << ypos << " - mouse" << std::endl;
    }
};

int main()
{
    TextEdit te;
    KeyInputListenerUi input_listener_ui(te);
    KeyInputListenerGameObject input_listener_go;
    input_listener_ui.set_next(&input_listener_go);

    CursorPositionListener cpl;
    MouseInputListener mml;

    auto& input = io::GLFWContext::get();
    input.set_listener(&input_listener_ui);
    input.set_listener(&cpl);
    input.set_listener(&mml);
    input.set_listener(&te);

    bool cursor = false;
    input_listener_go.add_callback(GLFW_KEY_T, GLFW_PRESS, 0, [&input, &cursor](){cursor = !cursor; input.set_cursor_mode(cursor);});

    while (input.update())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
