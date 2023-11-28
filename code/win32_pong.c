#define _UNICODE
#define UNICODE

#define FALSE 0
#define TRUE 1

#define INTERNAL static
#define LOCAL static
#define GLOBAL static

#define STRINGIFY(_s) #_s
#define TO_STRING(_x) STRINGIFY(_x)

#define ASSERT(_exp) if (!(_exp)) {\
MessageBoxW(0, L"Expression: " TO_STRING(_exp) L"\n\nAt line: " TO_STRING(__LINE__) L"\n\nIn: " __FILE__, L"Assertion Failed", (MB_ICONERROR | MB_OK));\
*((int *) 0) = 0;\
}

#include <windows.h>

GLOBAL int global_running = FALSE;

INTERNAL LRESULT CALLBACK win32_window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  LRESULT result;
  
  result = 0;
  switch(msg) {
    case WM_CLOSE: {
      DestroyWindow(window);
      break;
    }
    
    case WM_DESTROY: {
      global_running = FALSE;
      break;
    }
    
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
  // Window class validation
  {
    ATOM id_result;
    
    id_result = RegisterClassExW(&window_class);
    ASSERT(id_result != 0);
  }
  window = CreateWindowExW(0, window_class.lpszClassName, L"Game Window", (WS_OVERLAPPEDWINDOW | WS_VISIBLE), CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
  ASSERT(window != 0);
  // Game window message loop
  {
    MSG window_msg = {0};
    
    global_running = TRUE;
    while (global_running) {
      while (PeekMessageW(&window_msg, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&window_msg);
        DispatchMessageW(&window_msg);
      }
      
      // TODO: Setup a CPU framebuffer and paint a pixel on it
    };
  }
  return 0;
}