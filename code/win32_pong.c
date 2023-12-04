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

GLOBAL int global_running = FALSE;
GLOBAL Win32BackBuffer win32_back_buffer;

INTERNAL void WIN32_DEBUG_PRINT(wchar_t *msg, ...) {
  LOCAL wchar_t formated_msg[1024];
  va_list args;
  
  va_start(args, msg);
  vswprintf_s(formated_msg, ARRAY_COUNT(formated_msg), msg, args);
  va_end(args);
  OutputDebugStringW(formated_msg);
}

INTERNAL LRESULT CALLBACK win32_window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  LRESULT result;
  
  result = 0;
  switch(msg) {
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
  
  window_class.cbSize = sizeof(window_class);
  window_class.style = (CS_VREDRAW | CS_HREDRAW);
  window_class.hInstance = instance;
  window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
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
    window = CreateWindowExW(0, window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    ASSERT(window != 0, L"Invalid main window handle - Window couldn't be created by Windows!");
    SetWindowLongW(window, GWL_STYLE, GetWindowLongW(window, GWL_STYLE) & ~(WS_MAXIMIZEBOX));
  }
  
  /* Game window message loop */
  {
    MSG window_msg = {0};
    GameBackBuffer game_back_buffer = {0};
    GameInput game_input = {0};
    LARGE_INTEGER current_perf_counter = {0}, last_perf_counter = {0}, perf_frequency = {0};
    U32 sleep_granularity;
    float raw_elapsed_ms, cooked_elapsed_ms;
    float dt; /* this is actually 'last_frame_dt' */
    
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
              case VK_RETURN: {
                
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
        game_update_and_render(&game_back_buffer, &game_input);
        
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
          dt = 1.0f / fps; /* 'delta time' should be used when we want something to NOT BE frame rate dependent */
          
          raw_elapsed_ms = elapsed_ms;
          
          /* NOTE: CPU Sleep if too fast */
          {
            LOCAL F32 desired_ms_per_frame = 16.0f; /* aprox. equivalent to 60 FPS - TODO: get the monitor refresh rate later */
            LOCAL F32 elapsed_to_desired_ms_diff;
            
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
              cooked_elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / perf_frequency.QuadPart;
              /* NOTE: Debug 'time' print in the Visual Studio debugger */
              {
                LOCAL int print_counter = 1;
                
                ++print_counter;
                if (print_counter > 10) {
                  print_counter = 0;
                  WIN32_DEBUG_PRINT(L"Raw MS: %fms\tCooked MS: %fms\n", raw_elapsed_ms, cooked_elapsed_ms);
                }
              }
            }
            
          }
        }
        
        /* NOTE: This should be done last. First we 'wait' and then display the frame. */
        window_dc = GetDC(window);
        /*ASSERT(window_dc != 0);*/
        /* NOTE: BitBlt is faster than SctretchDIBits, maybe, in the future, change to BitBlt and do a bitmap resize by hand for the 'front buffer'? */
        StretchDIBits(window_dc, 0, 0, WIN32_FRONT_BUFFER_WIDTH, WIN32_FRONT_BUFFER_HEIGHT, 0, 0, game_back_buffer.width, game_back_buffer.height, game_back_buffer.memory, &win32_back_buffer.bmp_info, DIB_RGB_COLORS, SRCCOPY);
        if (window_dc != 0) { /* This check exist's because when the window minimized this is 0 */
          release_dc_result = ReleaseDC(window, window_dc);
          ASSERT(release_dc_result != 0, L"Windows failed to release the main window device context!");
        }
      }
      
      ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0, L"Couldn't get processor \'performance counter\' - \'QueryPerformanceCounter(...)\' shouldn't return 0!");
      last_perf_counter = current_perf_counter;
    };
  }
  return 0;
}