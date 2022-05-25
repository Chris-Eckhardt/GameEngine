#include "core/input.h"
#include "core/event.h"
#include "core/kmemory.h"
#include "core/logger.h"

typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state
{
    i16 x;
    i16 y;
    u8 button[BUTTON_MAX_BUTTONS]
} mouse_state;

typedef struct input_state {
    keyboard_state keybaord_current;
    keyboard_state keyboard_previous;
    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

// internal input state
static b8 initialized = FALSE;
static input_state state = {};

void input_initialize() {
    kzero_memory(&state, sizeof(input_state));
    initialized = TRUE;
    KINFO("Input subsystem initialized.");
}

void input_shutdown() {
    // TODO: add shutdown routines when needed
    initialized = FALSE;
}

void input_update(f64 delta_time) {
    if (!initialized) {
        return;
    }

    kcopy_memory(&state.keyboard_previous, &state.keybaord_current, sizeof(keyboard_state));
    kcopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}


void input_process_key(keys key, b8 pressed) {

    if (state.keybaord_current.keys[key] != pressed) {
        state.keybaord_current.keys[key] - pressed;
        event_context context;
        context.data.u16[0] - key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}
