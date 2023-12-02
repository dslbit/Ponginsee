#define _UNICODE
#define UNICODE

#define FALSE 0
#define TRUE 1

#define INTERNAL static
#define LOCAL static
#define GLOBAL static

#define STRINGIFY(_s) #_s
#define TO_STRING(_x) STRINGIFY(_x)

#if defined(DEBUG)
#define ASSERT(_exp) if (!(_exp)) {\
MessageBoxW(0, L"Expression: " TO_STRING(_exp) L"\n\nAt line: " TO_STRING(__LINE__) L"\n\nIn: " __FILE__, L"Assertion Failed", (MB_ICONERROR | MB_OK));\
*((int *) 0) = 0;\
}
#else
#define ASSERT(_exp)
#endif

#define CAST(_x) (_x)

#define ARRAY_LENGTH(_x) (sizeof((_x)) / sizeof((_x[0]))) /* in number of elements */
/* TODO: This can go away once 'game_layer' enters the room */
/* 640/360, 1280/720*/
#define FRONT_BUFFER_WIDTH (640)
#define FRONT_BUFFER_HEIGHT (360)
#define BACK_BUFFER_WIDTH (640)
#define BACK_BUFFER_HEIGHT (360)
#define BACK_BUFFER_BITS_PER_PIXEL (32)
#define BACK_BUFFER_BYTES_PER_PIXEL (BACK_BUFFER_BITS_PER_PIXEL / 8)
#define BACK_BUFFER_STRIDE (BACK_BUFFER_WIDTH * BACK_BUFFER_BYTES_PER_PIXEL)

#include <windows.h>
#include <strsafe.h>

/* TODO: Pack into a struct and move to a 'game_layer' */
GLOBAL int global_running = FALSE;
GLOBAL void *global_back_buffer_memory;
GLOBAL BITMAPINFO global_back_buffer_bmp_info;
GLOBAL BOOL move_up, move_down, move_left, move_right; /* W, S, A, D - respectively */
GLOBAL float dt; /* this is actually 'last_frame_dt' */

/* TODO: Move to a 'game_layer' */
INTERNAL void draw_pixel(int x, int y, int color) {
  LOCAL int *pixel;
  
  if (x > BACK_BUFFER_WIDTH || x < 0) return;
  if (y > BACK_BUFFER_HEIGHT || y < 0) return;
  pixel = CAST(int *) ((CAST(unsigned char *) global_back_buffer_memory) + (y * BACK_BUFFER_STRIDE) + (x * BACK_BUFFER_BYTES_PER_PIXEL));
  *pixel = color; /* AARRGGBB */
}

/* TODO: Move to a 'game_layer' */
INTERNAL void draw_rect(int x, int y, int width, int height, int color) {
  int i, j;
  LOCAL int *pixel;
  
  width = (x + width); /* make width be end_x */
  if (width < 0) width = 0;
  if (width > BACK_BUFFER_WIDTH) width = BACK_BUFFER_WIDTH;
  height = (y + height); /* make height be end_y */
  if (height < 0) height = 0;
  if (height > BACK_BUFFER_HEIGHT) height = BACK_BUFFER_HEIGHT;
  if (x < 0) x = 0;
  if (x >  BACK_BUFFER_WIDTH) x = BACK_BUFFER_WIDTH;
  if (y < 0) y = 0;
  if (y > BACK_BUFFER_HEIGHT) y = BACK_BUFFER_HEIGHT;
  for (i = y; i < height; ++i) {
    pixel =CAST(int *) ((CAST(unsigned char *) global_back_buffer_memory) + (i * BACK_BUFFER_STRIDE) + (x * BACK_BUFFER_BYTES_PER_PIXEL));
    for (j = x; j < width; ++j) {
      *pixel = color;
      pixel++;
    }
  }
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
      if (global_back_buffer_memory != 0) {
        VirtualFree(global_back_buffer_memory, 0, MEM_RELEASE);
      }
      /* Resize/Init back buffer */
      global_back_buffer_bmp_info.bmiHeader.biSize = sizeof(global_back_buffer_bmp_info.bmiHeader);
      global_back_buffer_bmp_info.bmiHeader.biWidth = BACK_BUFFER_WIDTH;
      global_back_buffer_bmp_info.bmiHeader.biHeight = -BACK_BUFFER_HEIGHT; /* the minus here means a top-left/top-down pixel origin */
      global_back_buffer_bmp_info.bmiHeader.biPlanes = 1;
      global_back_buffer_bmp_info.bmiHeader.biBitCount = BACK_BUFFER_BITS_PER_PIXEL;
      global_back_buffer_bmp_info.bmiHeader.biCompression = BI_RGB;
      back_buffer_mem_size = (BACK_BUFFER_WIDTH * BACK_BUFFER_HEIGHT) * BACK_BUFFER_BYTES_PER_PIXEL;
      global_back_buffer_memory = VirtualAlloc(0, back_buffer_mem_size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
      ASSERT(back_buffer_mem_size != 0);
    } break;
    
    /* TODO: Proper input */
    case WM_KEYDOWN: {
      DWORD key;
      
      key = CAST(DWORD) wparam;
      if (key == 'W') { move_up    = TRUE; }
      if (key == 'S') { move_down  = TRUE; }
      if (key == 'A') { move_left  = TRUE; }
      if (key == 'D') { move_right = TRUE; }
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
    ASSERT(id_result != 0);
  }
  /* Window creation */
  {
    int window_x, window_y, window_width, window_height;
    int monitor_width, monitor_height; /* in pixels */
    int error_result;
    DWORD window_styles;
    RECT rect = {0};
    
    rect.left = 0;
    rect.right = FRONT_BUFFER_WIDTH;
    rect.top = 0;
    rect.bottom = FRONT_BUFFER_HEIGHT;
    window_styles = (WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    error_result = AdjustWindowRect(&rect, window_styles, FALSE);
    window_width = rect.right - rect.left;
    window_height = rect.bottom - rect.top;
    ASSERT(error_result != 0);
    monitor_width = GetSystemMetrics(SM_CXSCREEN);
    ASSERT(monitor_width != 0);
    monitor_height = GetSystemMetrics(SM_CYSCREEN);
    ASSERT(monitor_height != 0);
    ASSERT(error_result != 0);
    window_x = (monitor_width - window_width) / 2;
    window_y = (monitor_height - window_height) / 2;
    window = CreateWindowExW(0, window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    SetWindowLongW(window, GWL_STYLE, GetWindowLongW(window, GWL_STYLE) & ~(WS_MAXIMIZEBOX));
  }
  
  ASSERT(window != 0);
  /* Game window message loop */
  {
    MSG window_msg = {0};
    float rect_x, rect_y;
    LOCAL LARGE_INTEGER current_perf_counter = {0}, last_perf_counter = {0}, perf_frequency = {0};
    
    rect_x = rect_y = 0;
    ASSERT(QueryPerformanceFrequency(&perf_frequency) != 0); /* this is the CPU ticks_per_second capability - TODO: maybe do some crazy shit if this fails */
    ASSERT(QueryPerformanceCounter(&last_perf_counter) != 0);
    global_running = TRUE;
    while (global_running) {
      /* Reset per frame input state */
      {
        move_up = move_down = move_left = move_right = FALSE;
      }
      
      while (PeekMessageW(&window_msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&window_msg);
        DispatchMessageW(&window_msg);
      }
      
      /* Update & Render */
      {
        LOCAL int release_dc_result;
        HDC window_dc;
        LOCAL float rect_move_speed; /* 10 meters per second */
        
        rect_move_speed = 5000.0f * dt;
        if (move_up)    { rect_y -= rect_move_speed; }
        if (move_down)  { rect_y += rect_move_speed; }
        if (move_left)  { rect_x -= rect_move_speed; }
        if (move_right) { rect_x += rect_move_speed; }
        
        /* Dirty clear background before drawing, TODO: a proper 'draw_background' */
        draw_rect(0, 0, BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT, 0x00000000);
        
        /* draw a rect at 100,100 */
        draw_rect(50 + CAST(int) rect_x, 50 + CAST(int) rect_y, 80, 45, 0xFFFF0000);
        
        window_dc = GetDC(window);
        /*ASSERT(window_dc != 0);*/
        /* NOTE: BitBlt is faster than SctretchDIBits, maybe, in the future, change to BitBlt and do a bitmap resize by hand for the 'front buffer'? */
        StretchDIBits(window_dc, 0, 0, FRONT_BUFFER_WIDTH, FRONT_BUFFER_HEIGHT, 0, 0, BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT, global_back_buffer_memory, &global_back_buffer_bmp_info, DIB_RGB_COLORS, SRCCOPY);
        if (window_dc != 0) { /* This check exist's because when the window minimized this is 0 */
          release_dc_result = ReleaseDC(window, window_dc);
          ASSERT(release_dc_result != 0);
        }
        
        /* Timing */
        {
          LOCAL LARGE_INTEGER perf_counter_diff = {0};
          LOCAL float elapsed_secs, elapsed_ms, elapsed_microsecs, elapsed_nanosecs = 0.0f;
          LOCAL int fps;
          
          ASSERT(QueryPerformanceCounter(&current_perf_counter) != 0); /* TODO: Maybe do something smart here if it fails for some reason */
          
          perf_counter_diff.QuadPart = current_perf_counter.QuadPart - last_perf_counter.QuadPart; /* counts elapsed */
          elapsed_secs = CAST(float) perf_counter_diff.QuadPart / CAST(float) perf_frequency.QuadPart; /* counts per second */
          elapsed_ms          = elapsed_secs * 1000.0f;
          elapsed_microsecs   = elapsed_secs * 1000000.0f;
          elapsed_nanosecs    = elapsed_secs * 1000000000.0f;
          fps = CAST(int) (1.0f / elapsed_secs); /*CAST(int) (perf_frequency.QuadPart / perf_counter_diff.QuadPart);*/
          dt = 1.0f / fps; /* 'delta time' should be used when we want something to NOT BE frame rate dependent */
          // NOTE: Debug 'time' print in the Visual Studio debugger
          {
            LOCAL int print_counter;
            wchar_t buffer[1024];
            
            ++print_counter;
            if (print_counter > 1000) {
              print_counter = 0;
              StringCbPrintfW(buffer, ARRAY_LENGTH(buffer), L"%dfps\t%fs\t%fms\t%fµs\t%fns\n", fps, elapsed_secs, elapsed_ms, elapsed_microsecs, elapsed_nanosecs);
              OutputDebugStringW(buffer);
            }
          }
          
          last_perf_counter = current_perf_counter;
        }
      }
      
      
    };
  }
  return 0;
}