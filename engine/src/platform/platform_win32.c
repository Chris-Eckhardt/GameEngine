#include "platform/platform.h"

// Windows platform layer.
#if KPLATFORM_WINDOWS

#include "core/logger.h"
#include "core/input.h"
#include "core/event.h"

#include "containers/darray.h"

#include <windows.h>
#include <windowsx.h>  // param input extraction
#include <Xinput.h> // for Gamepad
#include <stdlib.h>

// For surface creation
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>
#include "renderer/vulkan/vulkan_types.inl"

#pragma comment(lib, "XInput.lib") 

typedef struct internal_state {
    HINSTANCE h_instance;
    HWND hwnd;
    VkSurfaceKHR surface;
    b8 gamepads[XUSER_MAX_COUNT]; // x4
} internal_state;

// Clock
static f64 clock_frequency;
static LARGE_INTEGER start_time;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

void platform_process_gamepad_input(XINPUT_STATE* state);

b8 platform_startup(
    platform_state *plat_state,
    const char *application_name,
    i32 x,
    i32 y,
    i32 width,
    i32 height) {
    plat_state->internal_state = malloc(sizeof(internal_state));
    internal_state *state = (internal_state *)plat_state->internal_state;

    state->h_instance = GetModuleHandleA(0);

    // Setup and register window class.
    HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;  // Get double-clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = state->h_instance;
    wc.hIcon = icon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                   // Transparent
    wc.lpszClassName = "kohi_window_class";

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // Create window
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
    u32 window_ex_style = WS_EX_APPWINDOW;

    window_style |= WS_MAXIMIZEBOX;
    window_style |= WS_MINIMIZEBOX;
    window_style |= WS_THICKFRAME;

    // Obtain the size of the border.
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    // In this case, the border rectangle is negative.
    window_x += border_rect.left;
    window_y += border_rect.top;

    // Grow by the size of the OS border.
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(
        window_ex_style, "kohi_window_class", application_name,
        window_style, window_x, window_y, window_width, window_height,
        0, 0, state->h_instance, 0);

    if (handle == 0) {
        MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

        KFATAL("Window creation failed!");
        return false;
    } else {
        state->hwnd = handle;
    }

    // Show the window
    b32 should_activate = 1;  // TODO: if the window should not accept input, this should be false.
    i32 show_window_command_flags = should_activate ? SW_SHOW : SW_SHOWNOACTIVATE;
    // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
    // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
    ShowWindow(state->hwnd, show_window_command_flags);

    // Clock setup
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    clock_frequency = 1.0 / (f64)frequency.QuadPart;
    QueryPerformanceCounter(&start_time);

    // query for connected gamepads
    XInputEnable(true);

    i64 dwResult;    
    for (i16 i = 0; i < XUSER_MAX_COUNT; i++ )
    {
        XINPUT_STATE xiState;
        ZeroMemory(&xiState, sizeof(XINPUT_STATE));

        // set to not connected by default
        state->gamepads[i] = false;

        dwResult = XInputGetState(i, &xiState);

        if( dwResult == ERROR_SUCCESS )
        {
            state->gamepads[i] = true;
            KINFO("Gamepad connected at index %i", i);
        }
    }

    return true;
}

void platform_shutdown(platform_state *plat_state) {
    // Simply cold-cast to the known type.
    internal_state *state = (internal_state *)plat_state->internal_state;

    if (state->hwnd) {
        DestroyWindow(state->hwnd);
        state->hwnd = 0;
    }
}

b8 platform_pump_messages(platform_state *plat_state) {
    internal_state *state = (internal_state *)plat_state->internal_state;
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    // .. handle gamepad connection and state change
    i64 dwResult;    
    for (i16 i = 0; i < XUSER_MAX_COUNT; i++ )
    {
        XINPUT_STATE xiState;
        ZeroMemory(&xiState, sizeof(XINPUT_STATE));
        dwResult = XInputGetState(i, &xiState);

        if(dwResult == ERROR_SUCCESS) {
            if (!state->gamepads[i]) {
                state->gamepads[i] = true;
                KINFO("New gamepad detected at index %i", i);
            }
            // process input
            platform_process_gamepad_input(&xiState);
        } else {
            if (state->gamepads[i]) {
                state->gamepads[i] = false;
                KINFO("Connection to gamepad at index %i has been lost!", i);
            }
        }
    }

    return true;
}

void *platform_allocate(u64 size, b8 aligned) {
    return malloc(size);
}

void platform_free(void *block, b8 aligned) {
    free(block);
}

void *platform_zero_memory(void *block, u64 size) {
    return memset(block, 0, size);
}

void *platform_copy_memory(void *dest, const void *source, u64 size) {
    return memcpy(dest, source, size);
}

void *platform_set_memory(void *dest, i32 value, u64 size) {
    return memset(dest, value, size);
}

void platform_console_write(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, number_written, 0);
}

void platform_console_write_error(const char *message, u8 colour) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    static u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[colour]);
    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD number_written = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, number_written, 0);
}

f64 platform_get_absolute_time() {
    LARGE_INTEGER now_time;
    QueryPerformanceCounter(&now_time);
    return (f64)now_time.QuadPart * clock_frequency;
}

void platform_sleep(u64 ms) {
    Sleep(ms);
}

void platform_get_required_extension_names(const char ***names_darray) {
    darray_push(*names_darray, &"VK_KHR_win32_surface");
}

// Surface creation for Vulkan
b8 platform_create_vulkan_surface(platform_state *plat_state, vulkan_context *context) {
    // Simply cold-cast to the known type.
    internal_state *state = (internal_state *)plat_state->internal_state;

    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = state->h_instance;
    create_info.hwnd = state->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(context->instance, &create_info, context->allocator, &state->surface);
    if (result != VK_SUCCESS) {
        KFATAL("Vulkan surface creation failed.");
        return false;
    }

    context->surface = state->surface;
    return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        case WM_CLOSE:
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);
            return true;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            RECT r;
            GetClientRect(hwnd, &r);
            u32 width = r.right - r.left;
            u32 height = r.bottom - r.top;

            // Fire the event. The application layer should pick this up, but not handle it
            // as it shouldn be visible to other parts of the application.
            event_context context;
            context.data.u16[0] = (u16)width;
            context.data.u16[1] = (u16)height;
            event_fire(EVENT_CODE_RESIZED, 0, context);
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            // Key pressed/released
            b8 pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
            keys key = (u16)w_param;

            // Pass to the input subsystem for processing.
            input_process_key(key, pressed);
        } break;
        case WM_MOUSEMOVE: {
            // Mouse move
            i32 x_position = GET_X_LPARAM(l_param);
            i32 y_position = GET_Y_LPARAM(l_param);
            
            // Pass over to the input subsystem.
            input_process_mouse_move(x_position, y_position);
        } break;
        case WM_MOUSEWHEEL: {
            i32 z_delta = GET_WHEEL_DELTA_WPARAM(w_param);
            if (z_delta != 0) {
                // Flatten the input to an OS-independent (-1, 1)
                z_delta = (z_delta < 0) ? -1 : 1;
                input_process_mouse_wheel(z_delta);
            }
        } break;
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            b8 pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
            buttons mouse_button = BUTTON_MAX_BUTTONS;
            switch (msg) {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    mouse_button = BUTTON_LEFT;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    mouse_button = BUTTON_MIDDLE;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    mouse_button = BUTTON_RIGHT;
                    break;
            }

            // Pass over to the input subsystem.
            if (mouse_button != BUTTON_MAX_BUTTONS) {
                input_process_button(mouse_button, pressed);
            }
        } break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

void platform_process_gamepad_input(XINPUT_STATE* state) {
    i16 leftStickX = state->Gamepad.sThumbLX;
    i16 leftStickY = state->Gamepad.sThumbLY;
    i16 rightStickX = state->Gamepad.sThumbRX;
    i16 rightStickY = state->Gamepad.sThumbRY;
    
    // .. left stick
    if (leftStickX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 
    || leftStickY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
    || leftStickX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
    || leftStickY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        input_process_gamepad_left_stick_move(leftStickX, leftStickY);
    } else{
        input_process_gamepad_left_stick_move(0, 0);
    }

    // .. right stick
    if (rightStickX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 
    || rightStickY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
    || rightStickX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
    || rightStickY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
        input_process_gamepad_right_stick_move(rightStickX, rightStickY);
    } else {
        input_process_gamepad_right_stick_move(0, 0);
    }

    // .. Right & Left Triggers
    input_process_gamepad_trigger_left(state->Gamepad.bLeftTrigger);
    input_process_gamepad_trigger_right(state->Gamepad.bRightTrigger);

    // .. A
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_A) {
        input_process_gamepad_button(GAMEPAD_A, true);
    } else {
        input_process_gamepad_button(GAMEPAD_A, false);
    }

    // .. B
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_B) {
        input_process_gamepad_button(GAMEPAD_B, true);
    } else {
        input_process_gamepad_button(GAMEPAD_B, false);
    }

    // .. X
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_X) {
        input_process_gamepad_button(GAMEPAD_X, true);
    } else {
        input_process_gamepad_button(GAMEPAD_X, false);
    }

    // .. Y
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_Y) {
        input_process_gamepad_button(GAMEPAD_Y, true);
    } else {
        input_process_gamepad_button(GAMEPAD_Y, false);
    }

    // .. Left Sholder
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
        input_process_gamepad_button(GAMEPAD_LEFT_SHOLDER, true);
    } else {
        input_process_gamepad_button(GAMEPAD_LEFT_SHOLDER, false);
    }

    // .. Right Sholder
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
        input_process_gamepad_button(GAMEPAD_RIGHT_SHOLDER, true);
    } else {
        input_process_gamepad_button(GAMEPAD_RIGHT_SHOLDER, false);
    }

    // .. Left Stick Press
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) {
        input_process_gamepad_button(GAMEPAD_LEFT_STICK_PRESS, true);
    } else {
        input_process_gamepad_button(GAMEPAD_LEFT_STICK_PRESS, false);
    }

    // .. Right Stick Press
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) {
        input_process_gamepad_button(GAMEPAD_RIGHT_STICK_PRESS, true);
    } else {
        input_process_gamepad_button(GAMEPAD_RIGHT_STICK_PRESS, false);
    }

    // .. DPAD Up
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) {
        input_process_gamepad_button(GAMEPAD_DPAD_UP, true);
    } else {
        input_process_gamepad_button(GAMEPAD_DPAD_UP, false);
    }

    // .. DPAD Down
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
        input_process_gamepad_button(GAMEPAD_DPAD_DOWN, true);
    } else {
        input_process_gamepad_button(GAMEPAD_DPAD_DOWN, false);
    }

    // .. DPAD Left
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
        input_process_gamepad_button(GAMEPAD_DPAD_LEFT, true);
    } else {
        input_process_gamepad_button(GAMEPAD_DPAD_LEFT, false);
    }

    // .. DPAD Right
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
        input_process_gamepad_button(GAMEPAD_DPAD_RIGHT, true);
    } else {
        input_process_gamepad_button(GAMEPAD_DPAD_RIGHT, false);
    }

    // .. Start
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_START) {
        input_process_gamepad_button(GAMEPAD_START, true);
    } else {
        input_process_gamepad_button(GAMEPAD_START, false);
    }

    // .. Back
    if (state->Gamepad.wButtons & XINPUT_GAMEPAD_BACK) {
        input_process_gamepad_button(GAMEPAD_BACK, true);
    } else {
        input_process_gamepad_button(GAMEPAD_BACK, false);
    }
}

#endif  // KPLATFORM_WINDOWS