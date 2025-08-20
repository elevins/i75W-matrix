#pragma once

#include "common.hpp"

class BaseApp {
public:
    BaseApp() : sub_state(0) {}
    virtual ~BaseApp() = default;
    
    // Pure virtual methods that each app must implement
    virtual void draw(bool is_horizontal) = 0;
    virtual void handle_button_press(bool is_horizontal) = 0;
    virtual void reset_state() { sub_state = 0; }
    
    // Common methods
    int get_sub_state() const { return sub_state; }
    void set_sub_state(int state) { sub_state = state; }

protected:
    int sub_state;
};