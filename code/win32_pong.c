#include "pong.c"

#define _UNICODE
#define UNICODE

#if defined(WIN32_DEBUG)
#define ASSERT(_exp, _msg) if (!(_exp)) {\
MessageBoxW(0, L"Expression: " TO_STRING(_exp) L"\n" _msg L"\n\nAt line: " TO_STRING(__LINE__) L"\n\nIn: " __FILE__, L"Assertion Failed", (MB_ICONERROR | MB_OK));\
__debugbreak();\
*((int *) 0) = 0;\
}
#else
#define ASSERT(_exp)
#endif

/* 640/360, 1280/720*/
#define WIN32_FRONT_BUFFER_WIDTH (640)
#define WIN32_FRONT_BUFFER_HEIGHT (360)
#define WIN32_BACK_BUFFER_WIDTH (640)
#define WIN32_BACK_BUFFER_HEIGHT (360)
#define WIN32_BACK_BUFFER_BITS_PER_PIXEL (32)
#define WIN32_BACK_BUFFER_BYTES_PER_PIXEL (WIN32_BACK_BUFFER_BITS_PER_PIXEL / 8)
#define WIN32_BACK_BUFFER_STRIDE (WIN32_BACK_BUFFER_WIDTH * WIN32_BACK_BUFFER_BYTES_PER_PIXEL)

#include <windows.h>
#include <strsafe.h>
#include <wchar.h>

typedef struct Win32BackBuffer Win32BackBuffer;
struct Win32BackBuffer {
  BITMAPINFO bmp_info;
  void *memory;
  S32 width;
  S32 height;
};

typedef struct Win32GameCode Win32GameCode;
struct Win32GameCode {
  HMODULE dll;
  FILETIME dll_last_write_time;
  GameUpdateAndRenderFuncType *update_and_render;
  B32 is_valid;
};

/* TODO: What about no global variables? */
GLOBAL int global_running = FALSE;
GLOBAL Win32BackBuffer win32_back_buffer;
GLOBAL wchar_t global_path_to_exe_root[MAX_PATH];
GLOBAL S32 global_show_cursor;
GLOBAL B32 global_is_window_topmost;
GLOBAL B32 global_is_fullscreen;

INTERNAL void win32_debug_print(wchar_t *msg, ...) {
  LOCAL wchar_t formated_msg[1024];
  va_list args;
  
  va_start(args, msg);
  vswprintf_s(formated_msg, ARRAY_COUNT(formated_msg), msg, args);
  va_end(args);
  OutputDebugStringW(formated_msg);
}

INTERNAL void win32_build_root_path_for_file(wchar_t *full_path, S32 full_path_length, wchar_t *file_name) {
  StringCbCopyW(full_path, full_path_length, global_path_to_exe_root);
  StringCbCatW(full_path, full_path_length, L"\\");
  StringCbCatW(full_path, full_path_length, file_name);
}

INTERNAL Win32GameCode win32_load_game_code(wchar_t *dll_full_path, wchar_t *temp_dll_full_path, wchar_t *lock_pdb_full_path) {
  Win32GameCode game_code = {0};
  WIN32_FILE_ATTRIBUTE_DATA ignored = {0};
  
  if (!GetFileAttributesExW(lock_pdb_full_path, GetFileExInfoStandard, &ignored)) { /* NOTE: Only load if 'lock.tmp' is deleted - that is, after the pdb is unlock by the VS debugger */
    /* Getting the dll last written time */
    {
      WIN32_FILE_ATTRIBUTE_DATA dll_attributes = {0};
      
      ASSERT(GetFileAttributesExW(dll_full_path, GetFileExInfoStandard, &dll_attributes) != 0, L"Couldn't get the game dll file attributes!");
      game_code.dll_last_write_time = dll_attributes.ftLastWriteTime;
    }
    
    /* NOTE: This for loop is necessary because, for some unknown reason, CopyFile can fail on its own */
    for (;;) {
      if (CopyFile(dll_full_path, temp_dll_full_path, FALSE) != 0) break;
      if (GetLastError() == ERROR_FILE_NOT_FOUND) break;
    }
    game_code.dll = LoadLibraryW(temp_dll_full_path);
    if (game_code.dll != 0) {
      game_code.update_and_render = CAST(GameUpdateAndRenderFuncType *) GetProcAddress(game_code.dll, "game_update_and_render");
      game_code.is_valid = (game_code.update_and_render != 0);
    }
    
    /* @IMPORTANT: Remember to add checks here for every function in the game layer that should be exported */
    if (!game_code.is_valid) {
      game_code.update_and_render = game_update_and_render_stub;
    }
  }
  
  return game_code;
}

INTERNAL void win32_unload_game_code(Win32GameCode *game_code) {
  if (game_code->dll) {
    /* NOTE: For now, I'll unload the library, but what if it is still pointing to a string inside them? - Maybe also rename the dll with an index, keep track of it, and load the latest dll index and never unload the library, ever. */
    FreeLibrary(game_code->dll);
    game_code->dll = 0;
  }
  
  game_code->is_valid = FALSE;
  game_code->update_and_render = game_update_and_render_stub;
}

INTERNAL LRESULT CALLBACK win32_window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  LRESULT result;
  
  result = 0;
  switch(msg) {
    case WM_ACTIVATEAPP: {
      if (global_is_window_topmost) {
        if (wparam == TRUE) { /* if window is in focus, full alpha ON*/
          SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
        } else {
          SetLayeredWindowAttributes(window, RGB(0, 0, 0), 68, LWA_ALPHA);
        }
      }
    } break;
    
    case WM_SETCURSOR: {
      if (global_show_cursor) {
        SetCursor(LoadCursor(0, IDC_ARROW));
      } else {
        SetCursor(0);
      }
      
    } break;
    
    case WM_SIZE: {
      DWORD size_msg;
      size_t back_buffer_mem_size;
      
      size_msg = (DWORD) wparam;
      if (size_msg == SIZE_MAXHIDE || size_msg == SIZE_MINIMIZED) {
        break;
      }
      if (win32_back_buffer.memory != 0) {
        VirtualFree(win32_back_buffer.memory, 0, MEM_RELEASE);
      }
      /* Resize/Init back buffer */
      win32_back_buffer.bmp_info.bmiHeader.biSize = sizeof(win32_back_buffer.bmp_info.bmiHeader);
      win32_back_buffer.bmp_info.bmiHeader.biWidth = WIN32_BACK_BUFFER_WIDTH;
      win32_back_buffer.width = WIN32_BACK_BUFFER_WIDTH;
      win32_back_buffer.bmp_info.bmiHeader.biHeight = -WIN32_BACK_BUFFER_HEIGHT; /* the minus here means a top-left/top-down pixel origin */
      win32_back_buffer.height = WIN32_BACK_BUFFER_HEIGHT;
      win32_back_buffer.bmp_info.bmiHeader.biPlanes = 1;
      win32_back_buffer.bmp_info.bmiHeader.biBitCount = WIN32_BACK_BUFFER_BITS_PER_PIXEL;
      win32_back_buffer.bmp_info.bmiHeader.biCompression = BI_RGB;
      back_buffer_mem_size = (WIN32_BACK_BUFFER_WIDTH * WIN32_BACK_BUFFER_HEIGHT) * WIN32_BACK_BUFFER_BYTES_PER_PIXEL;
      win32_back_buffer.memory = VirtualAlloc(0, back_buffer_mem_size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
      ASSERT(back_buffer_mem_size != 0, L"Coun't allocate enough memory for the game \'back buffer\'!");
      
    } break;
    
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      ASSERT(0, L"Keyboard input shoudn't go though here!");
    } break;
    
    case WM_CLOSE: {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY: {
      global_running = FALSE;
    } break;
    
    default: {
      result = DefWindowProcW(window, msg, wparam, lparam);
    }
  }
  return result;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show) {
  WNDCLASSEXW window_class = {0};
  HWND window;
  
  /* Getting the executable file path */
  {
    int i, last_backslash_index;
    wchar_t path[MAX_PATH] = {0};
    
    ASSERT(GetModuleFileNameW(0, path, ARRAY_COUNT(path)) != 0, L"Couldn't ge the executable file path using 'GetModuleFileNameW(...)'!");
    last_backslash_index = 0;
    for (i = 0; (i < ARRAY_COUNT(path)) && (path[i] != '\0'); ++i) {
      if (path[i] == '\\') {
        last_backslash_index = i;
      }
    }
    ASSERT( ( ARRAY_COUNT(global_path_to_exe_root) == ARRAY_COUNT(path) ), L"Size of 'global_path_to_exe_root' should be the same as 'path'!");
    for (i = 0; i < last_backslash_index; ++i) {
      global_path_to_exe_root[i] = path[i];
    }
    global_path_to_exe_root[i] = '\0';
  }
  
  window_class.cbSize = sizeof(window_class);
  window_class.style = (CS_VREDRAW | CS_HREDRAW);
  window_class.hInstance = instance;
  /* window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); */
  //window_class.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  window_class.lpszClassName = L"Game Window Class";
  window_class.lpfnWndProc = win32_window_callback;
  /* Window class validation */
  {
    ATOM id_result;
    
    id_result = RegisterClassExW(&window_class);
    ASSERT(id_result != 0, L"Couldn't register the main window class for the game!");
  }
  
  /* Window creation */
  {
    int window_x, window_y, window_width, window_height;
    int monitor_width, monitor_height; /* in pixels */
    int error_result;
    DWORD window_styles;
    RECT rect = {0};
    
    /* @IMPORTANT: Will this work on a multi-monitor setup? Wouldn't be better to use GetDeviceCaps(...)? */
    rect.left = 0;
    rect.right = WIN32_FRONT_BUFFER_WIDTH;
    rect.top = 0;
    rect.bottom = WIN32_FRONT_BUFFER_HEIGHT;
    window_styles = (WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    error_result = AdjustWindowRect(&rect, window_styles, FALSE);
    ASSERT(error_result != 0, L"Couldn't \'AdjustWindowRect(...)\' for the main window!");
    window_width = rect.right - rect.left;
    window_height = rect.bottom - rect.top;
    monitor_width = GetSystemMetrics(SM_CXSCREEN);
    ASSERT(monitor_width != 0, L"Couldn't get \'monitor_width\' using \'GetSystemMetrics(...)\'!");
    monitor_height = GetSystemMetrics(SM_CYSCREEN);
    ASSERT(monitor_height != 0, L"Couldn't get \'monitor_height\' using \'GetSystemMetrics(...)\'!");
    window_x = (monitor_width - window_width) / 2;
    window_y = (monitor_height - window_height) / 2;
    global_is_window_topmost = FALSE;
    if (global_is_window_topmost) {
      window = CreateWindowExW((WS_EX_TOPMOST | WS_EX_LAYERED), window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    } else {
      window = CreateWindowExW(0, window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    }
    ASSERT(window != 0, L"Invalid main window handle - Window couldn't be created by Windows!");
    SetWindowLongW(window, GWL_STYLE, GetWindowLongW(window, GWL_STYLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX));
  }
  
  /* Game window message loop */
  {
    MSG window_msg = {0};
    GameBackBuffer game_back_buffer = {0};
    GameInput game_input = {0};
    GameState game_state = {0};
    Win32GameCode game_code = {0};
    LARGE_INTEGER current_perf_counter = {0}, last_perf_counter = {0}, perf_frequency = {0};
    U32 sleep_granularity;
    float raw_elapsed_ms, cooked_elapsed_ms;
    float dt; /* this is actually 'last_frame_dt', in seconds */
    wchar_t game_dll_full_path[MAX_PATH] = {0}, game_temp_dll_full_path[MAX_PATH] = {0}, lock_pdb_full_path[MAX_PATH] = {0};
    S32 monitor_refresh_rate;
    
    /* Get the monitor refresh rate */
    {
      HDC window_dc;
      
      window_dc = GetDC(window);
      monitor_refresh_rate = GetDeviceCaps(window_dc, VREFRESH); /* NOTE: User's primary monitor */
      ASSERT((monitor_refresh_rate >= 30), L"Your primary monitor is less than 30Hz, you wouldn't be able to experience the game well, so the application will be closed!"); /* TODO: Proper exit */
    }
    
    win32_build_root_path_for_file(game_dll_full_path, ARRAY_COUNT(game_dll_full_path), L"pong.dll");
    win32_build_root_path_for_file(game_temp_dll_full_path, ARRAY_COUNT(game_temp_dll_full_path), L"pong_temp.dll");
    win32_build_root_path_for_file(lock_pdb_full_path, ARRAY_COUNT(lock_pdb_full_path), L"lock.tmp");
    
    game_code = win32_load_game_code(game_dll_full_path, game_temp_dll_full_path, lock_pdb_full_path);
    
    /* NOTE: Trying to apply high resolution Windows timers for precise sleeping */
    {
      TIMECAPS machine_timing_caps;
      
      /* TODO: Actually this is fine if it fails, in the future just set to 1 by default, but for now I'll do this way in debug mode. */
      ASSERT(timeGetDevCaps(&machine_timing_caps, sizeof(machine_timing_caps)) == MMSYSERR_NOERROR, L"Couldn't get machine timing capabilities using 'timeGetDevCaps(...)'!");
      ASSERT(timeBeginPeriod(MIN(machine_timing_caps.wPeriodMin, machine_timing_caps.wPeriodMax)) == TIMERR_NOERROR, L"Couldn't set the Windows' timer resoltuion using 'timeBeginPeriod(...)'!");
      sleep_granularity = MIN(machine_timing_caps.wPeriodMin, machine_timing_caps.wPeriodMax);
    }
    ASSERT(QueryPerformanceFrequency(&perf_frequency) != 0, L"Couldn't get processor \'performance frequency\' - \'QueryPerformanceFrequency(...)\' shouldn't return 0!"); /* this is the CPU ticks_per_second capability - TODO: maybe do some crazy shit if this fails */
    ASSERT(QueryPerformanceCounter(&last_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!");
    dt = 0.0f;
    global_running = TRUE;
    while (global_running) {
      /* NOTE: Reload game code, if necessary */
      {
#if defined(WIN32_DEBUG)
        FILETIME current_dll_file_time = {0};
        WIN32_FILE_ATTRIBUTE_DATA dll_attributes = {0};
        
        ASSERT(GetFileAttributesExW(game_dll_full_path, GetFileExInfoStandard, &dll_attributes) != 0, L"Couldn't get the game dll file attributes!");
        current_dll_file_time = dll_attributes.ftLastWriteTime;
        if (CompareFileTime(&current_dll_file_time, &game_code.dll_last_write_time) != 0) {
          U32 load_try_index;
          
          win32_unload_game_code(&game_code);
          for (load_try_index = 0; (!game_code.is_valid) && (load_try_index < 100); ++load_try_index) {
            game_code = win32_load_game_code(game_dll_full_path, game_temp_dll_full_path, lock_pdb_full_path);
            Sleep(100);
          }
        }
#endif
      }
      
      /* NOTE: Input */
      while (PeekMessageW(&window_msg, 0, 0, 0, PM_REMOVE)) {
        switch (window_msg.message) {
          case WM_KEYDOWN:
          case WM_KEYUP:
          case WM_SYSKEYDOWN:
          case WM_SYSKEYUP: {
            DWORD key;
            S32 is_down, was_down, pressed, released;
            
            key = CAST(DWORD) window_msg.wParam;
            was_down = ( (window_msg.lParam & (1 << 30)) != 0 ); /* NOTE: Yes, Douglas, if it's not zero, not necessarily it needs to be 1, it can be different than 1. */
            is_down = ( (window_msg.lParam & (1 << 31)) == 0 );
            pressed = is_down;
            released = (was_down && !is_down) ? TRUE : FALSE;
            
            switch (key) {
              /* NOTE: This will also be used as the 'start' button for the game. */
              case VK_F11:
              case VK_RETURN: {
                B32 is_alt_pressed;
                B32 go_fullscreen;
                
                is_alt_pressed = (window_msg.lParam & (1 << 29)) != 0;
                go_fullscreen = FALSE;
                if (key == VK_F11 && released) {
                  go_fullscreen = TRUE;
                } else if (key == VK_RETURN && released && is_alt_pressed) {
                  go_fullscreen = TRUE;
                }
                
                if (go_fullscreen) {
                  win32_debug_print(L"Trying to go fullscreen...\n");
                  ASSERT(!global_is_window_topmost, L"Hey, man! This can be difficult to debug, turn it off.");
                  /* toggle fullscreen */
                  {
                    LOCAL WINDOWPLACEMENT prev_window_placement = {0}; /* TODO: move to win32 state */
                    DWORD window_styles;
                    
                    prev_window_placement.length = sizeof(prev_window_placement);
                    window_styles = GetWindowLongW(window, GWL_STYLE);
                    if (window_styles & WS_OVERLAPPEDWINDOW) {
                      MONITORINFO monitor_info = {0};
                      
                      monitor_info.cbSize = sizeof(monitor_info);
                      if ( (GetWindowPlacement(window, &prev_window_placement) && (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))) ) {
                        SetWindowLongW(window, GWL_STYLE,( window_styles & ~(WS_OVERLAPPEDWINDOW)));
                        SetWindowPos(window, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top, monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                                     monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top, (SWP_NOOWNERZORDER | SWP_FRAMECHANGED));
                        global_is_fullscreen = TRUE;
                      } else {
                        /* TODO: Let user now it failed to go fullscreen! */
                      }
                    } else {
                      SetWindowLongW(window, GWL_STYLE, (window_styles | WS_OVERLAPPEDWINDOW) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX));
                      SetWindowPlacement(window, &prev_window_placement);
                      SetWindowPos(window, NULL, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED));
                      global_is_fullscreen = FALSE;
                    }
                  }
                }
              } break;
              
              case VK_ESCAPE: {
                
              } break;
              
              case VK_UP:
              case 'W': {
                game_input.player1.up.pressed = pressed;
                game_input.player1.up.released = released;
              } break;
              
              case VK_DOWN:
              case 'S': {
                game_input.player1.down.pressed = pressed;
                game_input.player1.down.released = released;
              } break;
              
              case VK_LEFT:
              case 'A': {
                game_input.player1.left.pressed = pressed;
                game_input.player1.left.released = released;
              } break;
              
              case VK_RIGHT:
              case 'D': {
                game_input.player1.right.pressed = pressed;
                game_input.player1.right.released = released;
              } break;
              
              case VK_F4: {
                B32 is_alt_pressed;
                
                is_alt_pressed = (window_msg.lParam & (1 << 29)) != 0;
                if (is_alt_pressed) {
                  global_running = FALSE;
                };
              } break;
            }
            
          } break;
          
          default: {
            TranslateMessage(&window_msg);
            DispatchMessageW(&window_msg);
          }
        }
        
      }
      
      /* NOTE: Update & Render */
      {
        LOCAL int release_dc_result;
        HDC window_dc;
        
        game_back_buffer.width = win32_back_buffer.width;
        game_back_buffer.height = win32_back_buffer.height;
        game_back_buffer.bytes_per_pixel = WIN32_BACK_BUFFER_BYTES_PER_PIXEL;
        game_back_buffer.stride = game_back_buffer.width * game_back_buffer.bytes_per_pixel;
        game_back_buffer.memory = win32_back_buffer.memory;
        game_input.dt = dt;
        game_code.update_and_render(&game_back_buffer, &game_input, &game_state);
        
        /* Display the 'back_buffer' in window */
        {
          S32 front_buffer_width, front_buffer_height;
          S32 front_buffer_xoffset, front_buffer_yoffset;
          
          front_buffer_width = WIN32_FRONT_BUFFER_WIDTH;
          front_buffer_height = WIN32_FRONT_BUFFER_HEIGHT;
          front_buffer_xoffset = front_buffer_yoffset = 0;
          /* TODO: Store this only once, when user toggle fullscreen, in 'win32_state' */
          if (global_is_fullscreen) {
            int i;
            S32 monitor_max_width, monitor_max_height;
            MONITORINFO monitor_info = {0};
            /* NOTE: 16:9 resolutions divisible by 8 */
            LOCAL S32 front_buffer_dimensions[][2] = { { 128,  72 }, { 256,  144 }, { 384,  216 }, { 512,  288 }, { 640,  360 }, { 768,  432 }, { 896,  504 }, { 1024, 576 }, { 1152, 648 }, { 1280, 720 }, { 1408, 792 }, { 1536, 864 }, { 1664, 936 }, { 1792, 1008 }, { 1920, 1080 }, { 2048, 1152 }, { 2176, 1224 }, { 2304, 1296 }, { 2432, 1368 }, { 2560, 1440 }, { 2688, 1512 }, { 2816, 1584 }, { 2944, 1656 }, { 3072, 1728 }, { 3200, 1800 }, { 3328, 1872 }, { 3456, 1944 }, { 3584, 2016 }, { 3712, 2088 }, { 3840, 2160 }, { 3968, 2232 }, { 4096, 2304 }, { 4224, 2376 }, { 4352, 2448 }, { 4480, 2520 }, { 4608, 2592 }, { 4736, 2664 }, { 4864, 2736 }, { 4992, 2808 }, { 5120, 2880 }, { 5248, 2952 }, { 5376, 3024 }, { 5504, 3096 }, { 5632, 3168 }, { 5760, 3240 }, { 5888, 3312 }, { 6016, 3384 }, { 6144, 3456 }, { 6272, 3528 }, { 6400, 3600 }, { 6528, 3672 }, { 6656, 3744 }, { 6784, 3816 }, { 6912, 3888 }, { 7040, 3960 }, { 7168, 4032 }, { 7296, 4104 }, { 7424, 4176 }, { 7552, 4248 }, { 7680, 4320 } }; /* up to 8k */
            
            monitor_info.cbSize = sizeof(monitor_info);
            GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info);
            monitor_max_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
            monitor_max_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
            front_buffer_width = WIN32_BACK_BUFFER_WIDTH;
            front_buffer_height = WIN32_BACK_BUFFER_HEIGHT;
            for(i = 0; i < ARRAY_COUNT(front_buffer_dimensions); ++i) {
              if (front_buffer_dimensions[i][0] > monitor_max_width) break;
              if (front_buffer_dimensions[i][1] > monitor_max_height) break;
              front_buffer_width = front_buffer_dimensions[i][0];
              front_buffer_height = front_buffer_dimensions[i][1];
            }
            /* win32_debug_print(L"front_buffer width: %d, front_buffer height: %d \n", front_buffer_width, front_buffer_height);*/
            front_buffer_xoffset = (monitor_max_width - front_buffer_width) / 2;
            front_buffer_yoffset = (monitor_max_height - front_buffer_height) / 2;
            /* NOTE: Should I 'PatBlit(...)' the not-drawn regions? */
          }
          
          window_dc = GetDC(window); /* NOTE: Maybe this isn't necessary at all, since I'll do the rendering myself, I don't really need to release the DC everyframe. */
          /*ASSERT(window_dc != 0);*/
          /* NOTE: BitBlt is faster than SctretchDIBits, maybe, in the future, change to BitBlt and do a bitmap resize by hand for the 'front buffer'? */
          StretchDIBits(window_dc, front_buffer_xoffset, front_buffer_yoffset, front_buffer_width, front_buffer_height, 0, 0, game_back_buffer.width, game_back_buffer.height, game_back_buffer.memory, &win32_back_buffer.bmp_info, DIB_RGB_COLORS, SRCCOPY);
          if (window_dc != 0) { /* This check exist's because when the window minimized this is 0 */
            release_dc_result = ReleaseDC(window, window_dc);
            ASSERT(release_dc_result != 0, L"Windows failed to release the main window device context!");
          }
        }
        
        /* NOTE: Timing */
        {
          LOCAL LARGE_INTEGER perf_counter_diff = {0};
          LOCAL float elapsed_secs, elapsed_ms, elapsed_microsecs, elapsed_nanosecs = 0.0f;
          LOCAL int fps;
          
          ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!"); /* TODO: Maybe do something smart here if it fails for some reason */
          
          perf_counter_diff.QuadPart = current_perf_counter.QuadPart - last_perf_counter.QuadPart; /* counts elapsed */
          elapsed_secs = CAST(float) perf_counter_diff.QuadPart / CAST(float) perf_frequency.QuadPart; /* counts per second */
          elapsed_ms          = elapsed_secs * 1000.0f;
          elapsed_microsecs   = elapsed_secs * 1000000.0f;
          elapsed_nanosecs    = elapsed_secs * 1000000000.0f;
          fps = CAST(int) (1.0f / elapsed_secs); /*CAST(int) (perf_frequency.QuadPart / perf_counter_diff.QuadPart);*/
          
          raw_elapsed_ms = elapsed_ms;
          
          /* NOTE: CPU Sleep if too fast */
          {
            LOCAL F32 desired_ms_per_frame; /* 16.666 ~= 60 FPS, 33.333 ~= 30 FPS */
            LOCAL F32 elapsed_to_desired_ms_diff;
            
            desired_ms_per_frame = 1000.0f / CAST(F32) monitor_refresh_rate; /* @CLEANUP: no need to calc every frame */
            ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!");
            elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
            elapsed_to_desired_ms_diff = (desired_ms_per_frame - elapsed_ms);
            if (elapsed_ms < desired_ms_per_frame) {
              Sleep(CAST(S32) elapsed_to_desired_ms_diff);
              
              ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!");
              elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
              elapsed_to_desired_ms_diff = (desired_ms_per_frame - elapsed_ms);
              ASSERT(elapsed_to_desired_ms_diff < desired_ms_per_frame, L"Game loop slept for too long, that's bad!");
              while (elapsed_ms < desired_ms_per_frame) {
                ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!");
                elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
              }
            }
            cooked_elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / perf_frequency.QuadPart;
            dt = CAST(float) (current_perf_counter.QuadPart - last_perf_counter.QuadPart) / CAST(float) (perf_frequency.QuadPart); /* 'delta time' should be used when we want something to NOT BE frame rate dependent */
            last_perf_counter = current_perf_counter; /* NOTE: This should be here, right after the while loop to reach target ms per frame */
            
            /* NOTE: Debug 'time' print in the Visual Studio debugger */
            {
              LOCAL int print_counter = 1;
              
              ++print_counter;
              if (print_counter > 10) {
                print_counter = 0;
                win32_debug_print(L"Raw MS: %fms\tCooked MS: %fms\n", raw_elapsed_ms, cooked_elapsed_ms);
              }
            }
          }
        }
      }
    }
  }
  return 0;
}