#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"


typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct gamepad_state {
    b8 buttons[256];
    u8 leftTrigger;
    u8 rightTrigger;
    i16 leftStickX;
    i16 leftStickY;
    i16 rightStickX;
    i16 rightStickY;
} gamepad_state;

typedef struct input_state {
    keyboard_state keybaord_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
    gamepad_state gamepad_current;
    gamepad_state gamepad_previous;
} input_state;

// internal input state
static b8 initialized = false;
static input_state state = {};

void input_initialize() {
    kzero_memory(&state, sizeof(input_state));
    initialized = true;
    KINFO("Input subsystem initialized.");
}

void input_shutdown() {
    // TODO: add shutdown routines when needed
    initialized = false;
}

void input_update(f64 delta_time) {
    if (!initialized) {
        return;
    }

    kcopy_memory(&state.keyboard_previous, &state.keybaord_current, sizeof(keyboard_state));
    kcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
    kcopy_memory(&state.gamepad_previous, &state.gamepad_current, sizeof(gamepad_state));
}


void input_process_key(keys key, b8 pressed) {
    if (state.keybaord_current.keys[key] != pressed) {
        state.keybaord_current.keys[key] = pressed;
        event_context context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(buttons button, b8 pressed) {
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        // KDEBUG("Mouse pos: %i, %i!", x, y);
        state.mouse_current.x = x;
        state.mouse_current.y = y;
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 input_is_key_down(keys key) {
    if (!initialized) {
        return false;
    }
    return state.keybaord_current.keys[key] == true;
}

b8 input_is_key_up(keys key) {
    if (!initialized) {
        return true;
    }
    return state.keybaord_current.keys[key] == false;
}

b8 input_was_key_down(keys key) {
    if (!initialized) {
        return false;
    }
    return state.keyboard_previous.keys[key] == true;
}

b8 input_was_key_up(keys key) {
    if (!initialized) {
        return true;
    }
    return state.keyboard_previous.keys[key] == false;
}

b8 input_was_button_down(buttons button) {
    if (!initialized) {
        return false;
    }
    return state.mouse_previous.buttons[button] == true;
}

b8 input_was_button_up(buttons button) {
    if (!initialized) {
        return true;
    }
    return state.mouse_previous.buttons[button] == false;
}

void input_get_mouse_position(i32* x, i32* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_position(i32* x, i32* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}

b8 input_is_gamepad_button_down(gamepad_buttons button) {
    if (!initialized) {
        return false;
    }
    return state.gamepad_current.buttons[button] == true;
}

b8 input_is_gamepad_button_up(gamepad_buttons button) {
    if (!initialized) {
        return true;
    }
    return state.gamepad_current.buttons[button] == false;
}

b8 input_was_gamepad_button_down(gamepad_buttons button) {
    if (!initialized) {
        return false;
    }
    return state.gamepad_previous.buttons[button] == true;
}

b8 input_was_gamepad_button_up(gamepad_buttons button) {
    if (!initialized) {
        return true;
    }
    return state.gamepad_previous.buttons[button] == false;
}

void input_get_gamepad_left_stick_position(i16* x, i16* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.gamepad_current.leftStickX;
    *y = state.gamepad_current.leftStickY;
}

void input_get_previous_gamepad_left_stick_position(i16* x, i16* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.gamepad_previous.leftStickX;
    *y = state.gamepad_previous.leftStickY;
}

void input_get_gamepad_right_stick_position(i16* x, i16* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.gamepad_current.rightStickX;
    *y = state.gamepad_current.rightStickY;
}

void input_get_previous_gamepad_right_stick_position(i16* x, i16* y) {
    if (!initialized) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = state.gamepad_previous.rightStickX;
    *y = state.gamepad_previous.rightStickY;
}

void input_process_gamepad_left_stick_move(i16 x, i16 y) {
    if (state.gamepad_current.leftStickX != x || state.gamepad_current.leftStickY != y) {
        KDEBUG("gamepad left stick pos: %i, %i!", x, y);
        state.gamepad_current.leftStickX = x;
        state.gamepad_current.leftStickY = y;
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_GAMEPAD_LEFT_STICK_MOVED, 0, context);
    }
}

void input_process_gamepad_right_stick_move(i16 x, i16 y) {
    if (state.gamepad_current.rightStickX != x || state.gamepad_current.rightStickY != y) {
        KDEBUG("gamepad right stick pos: %i, %i!", x, y);
        state.gamepad_current.rightStickX = x;
        state.gamepad_current.rightStickY = y;
        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_GAMEPAD_RIGHT_STICK_MOVED, 0, context);
    }
}

void input_process_gamepad_button(gamepad_buttons button, b8 pressed) {
    if (state.gamepad_current.buttons[button] != pressed) {
        KDEBUG("gamepad button %s!", pressed ? "PRESSED" : "RELEASED");
        state.gamepad_current.buttons[button] = pressed;
        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_GAMEPAD_BUTTON_PRESSED : EVENT_CODE_GAMEPAD_BUTTON_RELEASED, 0, context);
    }
}

void input_process_gamepad_trigger_right(u8 value) {
    if (state.gamepad_current.rightTrigger != value) {
        KDEBUG("gamepad right trigger value: %i", value);
        state.gamepad_current.rightTrigger = value;
        event_context context;
        context.data.u16[0] = value;
        event_fire(EVENT_CODE_GAMEPAD_RIGHT_TRIGGER_VALUE_CHANGED, 0, context);
    }
}

void input_process_gamepad_trigger_left(u8 value) {
    if (state.gamepad_current.leftTrigger != value) {
        KDEBUG("gamepad left trigger value: %i", value);
        state.gamepad_current.leftTrigger = value;
        event_context context;
        context.data.u16[0] = value;
        event_fire(EVENT_CODE_GAMEPAD_LEFT_TRIGGER_VALUE_CHANGED, 0, context);
    }
}