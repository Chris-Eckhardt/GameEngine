#include "application.h"
#include "game_types.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/kmemory.h"
#include "core/event.h"
#include "core/input.h"

typedef struct application_state {
    game* game_inst;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;
} application_state;

static b8 initialized = false;
static application_state app_state;

// .. Event Handlers
b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context);
b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context);

/**
 * @brief 
 * 
 * @param game_inst 
 * @return b8 
 */
b8 application_create(game* game_inst) {
    if (initialized) {
        KERROR("application_create called more than once.");
        return false;
    }

    app_state.game_inst = game_inst;

    // initialize subsystems
    initialize_logging();
    input_initialize();

    KFATAL("Test message: %f", 3.14f);
    KERROR("Test message: %f", 3.14f);
    KWARN("Test message: %f", 3.14f);
    KINFO("Test message: %f", 3.14f);
    KDEBUG("Test message: %f", 3.14f);
    KTRACE("Test message: %f", 3.14f);

    app_state.is_running = true;
    app_state.is_suspended = false;

    if (!event_initialize()) {
        KERROR("Event system failed initialization. Applicaiton cannot continue.")
        return false;
    }

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    if (!platform_startup(
        &app_state.platform, 
        game_inst->app_config.name, 
        game_inst->app_config.start_pos_x, 
        game_inst->app_config.start_pos_y, 
        game_inst->app_config.start_width,
        game_inst->app_config.start_height)) {
            return false;
    }
    
    if (!app_state.game_inst->initialize(app_state.game_inst)) {
        KFATAL("Game failed to initialize!")
        return false;
    }

    app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);

    initialized = true;

    return true;
}

/**
 * @brief 
 * 
 * @return b8 
 */
b8 application_run() {

    KINFO(get_memory_usage_str());

    while (app_state.is_running) {
        if (!platform_pump_messages(&app_state.platform)) {
            app_state.is_running = false;
        }

        if (!app_state.is_suspended) {
            if (!app_state.game_inst->update(app_state.game_inst, (f32)0)) {
                KFATAL("Game update failed, shutting down.");
                app_state.is_running = false;
                break;
            }

            if (!app_state.game_inst->render(app_state.game_inst, (f32)0)) {
                KFATAL("Game render failed, shutting down.");
                app_state.is_running = false;
                break;
            }
        }

        input_update(0);
    }

    app_state.is_running = false;

    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    event_shutdown();
    input_shutdown();

    platform_shutdown(&app_state.platform);

    return true;
}

b8 application_on_event(u16 code, void* sender, void* listener_inst, event_context context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            KINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down. \n");
            app_state.is_running = false;
            return true;
        }
    }
    return false;
}

b8 application_on_key(u16 code, void* sender, void* listener_inst, event_context context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE) {
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return true;
        } else if (key_code == KEY_A) {
            KDEBUG("Explicit - A key pressed!");
        } else {
            KDEBUG("'%c' key pressed in window.", key_code);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B) {
            KDEBUG("Explicit - B key relesed!");
        } else {
            KDEBUG("'%c' key released in window.", key_code);
        }
    }
    return false;
}