#include "..\game\pong.c"

/*
-* TODO: Cleanup Win32 locals?
-*
-* NOTE: This platform layer doesn't assume the game will have a flexible-free
-* aspect ratio. The *back buffer* has a fixed 16:9 small aspect ratio
-* resolution that can be scaled up in the *front buffer* before drawing when
-* going fullscreen. Under the hood everything is rendered in the base
-* resolution.
*/

#define _UNICODE
#define UNICODE

#if defined(WIN32_DEBUG)

#if defined(ASSERT)
#undef ASSERT
#endif

#define ASSERT(_exp, _msg) if (!(_exp)) {\
MessageBoxW(0, L"Expression: " STRINGIFY(_exp) L"\n" _msg L"\n\nAt line: " STRINGIFY(__LINE__) L"\n\nIn: " __FILE__, L"Assertion Failed", (MB_ICONERROR | MB_OK));\
__debugbreak();\
*((int *) 0) = 0;\
}
#else
#if defined(ASSERT)
#undef ASSERT
#endif
#define ASSERT(_exp, _msg)
#endif /* defined(WIN32_DEBUG) */

#define WIN32_ID_TIMER_MAIN_FIBER 1
#define WIN32_MAIN_WINDOW_STYLES (WS_VISIBLE)
#define WIN32_MAIN_WINDOW_REMOVED_STYLES (~(WS_CAPTION))

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
#include <windowsx.h>

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

typedef struct Win32State Win32State;
struct Win32State { /* TODO: add game memory info */
  B32 is_running; /* main game loop */
  B32 is_cursor_visible;
  B32 is_window_topmost;
  B32 is_fullscreen;
  Win32BackBuffer back_buffer;
  S32 front_buffer_xoffset;
  S32 front_buffer_yoffset;
  S32 front_buffer_width;
  S32 front_buffer_height;
  wchar_t root_path[MAX_PATH]; /* path to '.exe' root directory */
  WINDOWPLACEMENT prev_window_placement;
  HANDLE fiber_main;
  HANDLE fiber_message;
};

GLOBAL Win32State global_state;

INTERNAL void win32_debug_print(wchar_t *msg, ...) {
  LOCAL wchar_t formated_msg[1024];
  va_list args;
  
  va_start(args, msg);
  vswprintf_s(formated_msg, ARRAY_COUNT(formated_msg), msg, args);
  va_end(args);
  OutputDebugStringW(formated_msg);
}

INTERNAL void win32_build_root_path_for_file(wchar_t *full_path, S32 full_path_length, wchar_t *file_name) {
  StringCbCopyW(full_path, full_path_length, global_state.root_path);
  StringCbCatW(full_path, full_path_length, L"\\");
  StringCbCatW(full_path, full_path_length, file_name);
}


PLATFORM_FREE_ENTIRE_FILE_PROTOTYPE(win32_debug_free_entire_file) {
  if (address) {
    VirtualFree(address, 0, MEM_RELEASE);
  }
}

PLATFORM_READ_ENTIRE_FILE_PROTOTYPE(win32_debug_read_entire_file) {
  HANDLE file_handle;
  LARGE_INTEGER file_size;
  ReadFileResult result = {0};
  wchar_t file_full_path[MAX_PATH];
  
  win32_build_root_path_for_file(file_full_path, ARRAY_COUNT(file_full_path), file_name);
  file_handle = CreateFileW(file_full_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (file_handle == INVALID_HANDLE_VALUE) {
    ASSERT(file_handle != INVALID_HANDLE_VALUE, L"Failed to open a file for reading.");
    return result;
  }
  if (GetFileSizeEx(file_handle, &file_size) == 0) {
    ASSERT(0, L"Failed to get the file size.");
  }
  result.data_size = file_size.QuadPart;
  result.data = VirtualAlloc(0, file_size.QuadPart, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE); /* NOTE: because this is a debug function, this isn't a great deal */
  if (result.data != 0) {
    DWORD bytes_read;
    if (ReadFile(file_handle, result.data, truncate_u64_to_u32(file_size.QuadPart), &bytes_read, 0) == 0) {
      ASSERT(0, L"Failed to read the file.");
    }
  }
  CloseHandle(file_handle);
  return result;
}

PLATFORM_WRITE_ENTIRE_FILE_PROTOTYPE(win32_debug_write_entire_file) {
  HANDLE file_handle;
  BOOL result;
  DWORD bytes_written;
  wchar_t file_full_path[MAX_PATH];
  
  win32_build_root_path_for_file(file_full_path, ARRAY_COUNT(file_full_path), file_name);
  file_handle = CreateFileW(file_full_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
  if (file_handle == INVALID_HANDLE_VALUE) {
    ASSERT(file_handle != INVALID_HANDLE_VALUE, L"Failed to open a file handle for writing data.");
    return 0;
  }
  bytes_written = 0;
  result = WriteFile(file_handle, data, data_size, &bytes_written, 0);
  ASSERT(data_size == bytes_written, L"Failed to write to the file.");
  CloseHandle(file_handle);
  return result;
}

INTERNAL Win32GameCode win32_load_game_code(wchar_t *dll_full_path, wchar_t *temp_dll_full_path, wchar_t *lock_pdb_full_path) {
  Win32GameCode game_code = {0};
  WIN32_FILE_ATTRIBUTE_DATA ignored = {0};
  
  if (!GetFileAttributesExW(lock_pdb_full_path, GetFileExInfoStandard, &ignored)) { /* NOTE: Only load if 'lock.tmp' is deleted - that is, after the pdb is unlock by the VS debugger */
    /* Getting the dll last written time */
    {
      WIN32_FILE_ATTRIBUTE_DATA dll_attributes = {0};
      DWORD result;
      
      result = GetFileAttributesExW(dll_full_path, GetFileExInfoStandard, &dll_attributes);
      ASSERT(result != 0, L"Couldn't get the game dll file attributes!");
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
      game_code.update_and_render = 0;
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
  game_code->update_and_render = 0;
}

INTERNAL void win32_change_key_state(GameButtonState *button, B32 pressed, B32 released) {
  ASSERT(button != 0, L"'win32_key_event_helper' failed! Null 'button'!");
  button->pressed = pressed;
  button->released = released;
}

INTERNAL LRESULT CALLBACK win32_window_callback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
  LRESULT result;
  GameInput *game_input;
  
  result = 0;
  game_input = CAST(GameInput *) GetWindowLongPtrW(window, GWLP_USERDATA);
  switch(msg) {
    
    /* TODO: Set lower 'target_seconds_per_frame' when app is not in focus */
    case WM_ACTIVATEAPP: {
      if (global_state.is_window_topmost) {
        if (wparam == TRUE) { /* if window is in focus, full alpha ON*/
          SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
        } else {
          SetLayeredWindowAttributes(window, RGB(0, 0, 0), 68, LWA_ALPHA);
        }
      }
    } break;
    
    case WM_SETCURSOR: {
      if (global_state.is_cursor_visible) {
        SetCursor(LoadCursor(0, IDC_ARROW));
      } else {
        SetCursor(0);
      }
      
    } break;
    
    case WM_TIMER: {
      SwitchToFiber(global_state.fiber_main);
    } break;
    
    case WM_ENTERMENULOOP:
    case WM_ENTERSIZEMOVE: {
      SetTimer(window, WIN32_ID_TIMER_MAIN_FIBER, 1, 0);
    } break;
    
    case WM_EXITMENULOOP:
    case WM_EXITSIZEMOVE: {
      KillTimer(window, WIN32_ID_TIMER_MAIN_FIBER);
    } break;
    
    case WM_SIZE: {
      DWORD size_msg;
      size_t back_buffer_mem_size;
      
      size_msg = (DWORD) wparam;
      if (size_msg == SIZE_MAXHIDE || size_msg == SIZE_MINIMIZED) {
        break;
      }
      if (global_state.back_buffer.memory != 0) {
        VirtualFree(global_state.back_buffer.memory, 0, MEM_RELEASE);
      }
      /* Resize/Init back buffer */
      global_state.back_buffer.bmp_info.bmiHeader.biSize = sizeof(global_state.back_buffer.bmp_info.bmiHeader);
      global_state.back_buffer.bmp_info.bmiHeader.biWidth = WIN32_BACK_BUFFER_WIDTH;
      global_state.back_buffer.width = WIN32_BACK_BUFFER_WIDTH;
      global_state.back_buffer.bmp_info.bmiHeader.biHeight = -WIN32_BACK_BUFFER_HEIGHT; /* the minus here means a top-left/top-down pixel origin */
      global_state.back_buffer.height = WIN32_BACK_BUFFER_HEIGHT;
      global_state.back_buffer.bmp_info.bmiHeader.biPlanes = 1;
      global_state.back_buffer.bmp_info.bmiHeader.biBitCount = WIN32_BACK_BUFFER_BITS_PER_PIXEL;
      global_state.back_buffer.bmp_info.bmiHeader.biCompression = BI_RGB;
      back_buffer_mem_size = (WIN32_BACK_BUFFER_WIDTH * WIN32_BACK_BUFFER_HEIGHT) * WIN32_BACK_BUFFER_BYTES_PER_PIXEL;
      global_state.back_buffer.memory = VirtualAlloc(0, back_buffer_mem_size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
      ASSERT(back_buffer_mem_size != 0, L"Coun't allocate enough memory for the game \'back buffer\'!");
      global_state.front_buffer_width = WIN32_FRONT_BUFFER_WIDTH;
      global_state.front_buffer_height = WIN32_FRONT_BUFFER_HEIGHT;
      global_state.front_buffer_xoffset = 0;
      global_state.front_buffer_yoffset = 0;
    } break;
    
    /* @IMPORTANT: This will not work properly if the front buffer is resized - TODO: Full screen mouse position handling, maybe set cursor pos in middle of the monitor and draw a virtual cursor that moves based on cursor delta */
    case WM_MOUSEMOVE: {
      game_input->mouse_pos.x = CAST(F32) GET_X_LPARAM(lparam);
      game_input->mouse_pos.y = CAST(F32) GET_Y_LPARAM(lparam);
    } break;
    
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
      DWORD key;
      B32 is_down, was_down, pressed, released;
      
      if (game_input == 0) {
        break;
      }
      
      key = CAST(DWORD) wparam;
      was_down = ( (lparam & (1 << 30)) != 0 ); /* NOTE: Yes, Douglas, if it's not zero, not necessarily it needs to be 1, it can be different than 1. */
      is_down = ( (lparam & (1 << 31)) == 0 );
      pressed = is_down;
      released = (was_down && !is_down) ? TRUE : FALSE;
      
      switch (key) {
        case VK_F3: {
          win32_change_key_state(&game_input->player1.f3, pressed, released);
        } break;
        
        case VK_NEXT: {
          win32_change_key_state(&game_input->player1.plus, pressed, released);
        } break;
        
        case VK_PRIOR: {
          win32_change_key_state(&game_input->player1.minus, pressed, released);
        } break;
        
        /* NOTE: This will also be used as the 'start' button for the game (VK_RETURN). */
        case VK_F11:
        case VK_RETURN:{
          B32 is_alt_pressed;
          B32 go_fullscreen;
          
          is_alt_pressed = (lparam & (1 << 29)) != 0;
          go_fullscreen = FALSE;
          if (key == VK_RETURN && is_alt_pressed == FALSE) {
            win32_change_key_state(&game_input->player1.start, pressed, released);
          } else if (key == VK_F11 && released) {
            go_fullscreen = TRUE;
          } else if (key == VK_RETURN && released && is_alt_pressed) {
            go_fullscreen = TRUE;
          }
          
          /* NOTE: Toggle fullscreen - TODO: Pull out code and make a func() to start game full screen */
          if (go_fullscreen) {
            ASSERT(!global_state.is_window_topmost, L"Hey, man! This can be difficult to debug, turn it off.");
            {
              DWORD window_styles;
              
              global_state.prev_window_placement.length = sizeof(global_state.prev_window_placement);
              window_styles = GetWindowLongW(window, GWL_STYLE);
              if (window_styles && !global_state.is_fullscreen) {
                MONITORINFO monitor_info = {0};
                
                monitor_info.cbSize = sizeof(monitor_info);
                if ( (GetWindowPlacement(window, &global_state.prev_window_placement) && (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor_info))) ) {
                  SetWindowPos(window, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top, monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                               monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top, (SWP_NOOWNERZORDER | SWP_FRAMECHANGED));
                  global_state.is_fullscreen = TRUE;
                  
                  /* NOTE: Calc new front_buffer dimensions */
                  {
                    int i;
                    S32 monitor_max_width, monitor_max_height;
                    HDC window_dc;
                    /* NOTE: 16:9 resolutions divisible by 8 */
                    LOCAL S32 front_buffer_dimensions[][2] = { { 128,  72 }, { 256,  144 }, { 384,  216 }, { 512,  288 }, { 640,  360 }, { 768,  432 }, { 896,  504 }, { 1024, 576 }, { 1152, 648 }, { 1280, 720 }, { 1408, 792 }, { 1536, 864 }, { 1664, 936 }, { 1792, 1008 }, { 1920, 1080 }, { 2048, 1152 }, { 2176, 1224 }, { 2304, 1296 }, { 2432, 1368 }, { 2560, 1440 }, { 2688, 1512 }, { 2816, 1584 }, { 2944, 1656 }, { 3072, 1728 }, { 3200, 1800 }, { 3328, 1872 }, { 3456, 1944 }, { 3584, 2016 }, { 3712, 2088 }, { 3840, 2160 }, { 3968, 2232 }, { 4096, 2304 }, { 4224, 2376 }, { 4352, 2448 }, { 4480, 2520 }, { 4608, 2592 }, { 4736, 2664 }, { 4864, 2736 }, { 4992, 2808 }, { 5120, 2880 }, { 5248, 2952 }, { 5376, 3024 }, { 5504, 3096 }, { 5632, 3168 }, { 5760, 3240 }, { 5888, 3312 }, { 6016, 3384 }, { 6144, 3456 }, { 6272, 3528 }, { 6400, 3600 }, { 6528, 3672 }, { 6656, 3744 }, { 6784, 3816 }, { 6912, 3888 }, { 7040, 3960 }, { 7168, 4032 }, { 7296, 4104 }, { 7424, 4176 }, { 7552, 4248 }, { 7680, 4320 } }; /* up to 8k */
                    
                    monitor_max_width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
                    monitor_max_height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
                    global_state.front_buffer_width = WIN32_BACK_BUFFER_WIDTH;
                    global_state.front_buffer_height = WIN32_BACK_BUFFER_HEIGHT;
                    for(i = 0; i < ARRAY_COUNT(front_buffer_dimensions); ++i) {
                      if (front_buffer_dimensions[i][0] > monitor_max_width) break;
                      if (front_buffer_dimensions[i][1] > monitor_max_height) break;
                      global_state.front_buffer_width = front_buffer_dimensions[i][0];
                      global_state.front_buffer_height = front_buffer_dimensions[i][1];
                    }
                    /* win32_debug_print(L"front_buffer width: %d, front_buffer height: %d \n", front_buffer_width, front_buffer_height);*/
                    global_state.front_buffer_xoffset = (monitor_max_width - global_state.front_buffer_width) / 2;
                    global_state.front_buffer_yoffset = (monitor_max_height - global_state.front_buffer_height) / 2;
                    /* NOTE: It will only use PatBlt if 'front_buffer' has an xoffset or yoffset - @IMPORTANT: Is this necessary every frame or only when going fullscreen? */
                    if (global_state.front_buffer_xoffset > 0 || global_state.front_buffer_yoffset > 0) {
                      window_dc = GetDC(window);
                      /* top */
                      PatBlt(window_dc, 0, 0, monitor_max_width, global_state.front_buffer_yoffset, BLACKNESS); 
                      /* bottom */
                      PatBlt(window_dc, 0, monitor_max_height - global_state.front_buffer_yoffset, monitor_max_width, monitor_max_height - global_state.front_buffer_yoffset - 1, BLACKNESS); 
                      /* left */
                      PatBlt(window_dc, 0, 0, global_state.front_buffer_xoffset, monitor_max_height, BLACKNESS);
                      /* right */
                      PatBlt(window_dc, monitor_max_width - global_state.front_buffer_xoffset, 0, monitor_max_width - global_state.front_buffer_xoffset - 1, monitor_max_height, BLACKNESS);
                      ReleaseDC(window, window_dc);
                    }
                  }
                  win32_debug_print(L"Fullscreen toggle [ON]\n");
                } else {
                  /* TODO: Let user know it failed to go fullscreen! - Maybe a notification-area msg? */
                }
              } else {
                SetWindowLongW(window, GWL_STYLE, WIN32_MAIN_WINDOW_STYLES & WIN32_MAIN_WINDOW_REMOVED_STYLES);
                SetWindowPlacement(window, &global_state.prev_window_placement);
                SetWindowPos(window, NULL, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED));
                global_state.is_fullscreen = FALSE;
                global_state.front_buffer_width = WIN32_FRONT_BUFFER_WIDTH;
                global_state.front_buffer_height = WIN32_FRONT_BUFFER_HEIGHT;
                global_state.front_buffer_xoffset = 0;
                global_state.front_buffer_yoffset = 0;
                win32_debug_print(L"Fullscreen toggle [OFF]\n");
              }
            }
          }
        } break;
        
        case VK_ESCAPE: {
          win32_change_key_state(&game_input->player1.back, pressed, released);
        } break;
        
        case VK_UP:
        case 'W': {
          win32_change_key_state(&game_input->player1.up, pressed, released);
        } break;
        
        case VK_DOWN:
        case 'S': {
          win32_change_key_state(&game_input->player1.down, pressed, released);
        } break;
        
        case VK_LEFT:
        case 'A': {
          win32_change_key_state(&game_input->player1.left, pressed, released);
        } break;
        
        case VK_RIGHT:
        case 'D': {
          win32_change_key_state(&game_input->player1.right, pressed, released);
        } break;
        
        case VK_F4: {
          B32 is_alt_pressed;
          
          is_alt_pressed = (lparam & (1 << 29)) != 0;
          if (is_alt_pressed) {
            global_state.is_running = FALSE;
          };
        } break;
        
        case '0': { win32_change_key_state(&game_input->player1.aux0, pressed, released); } break;
        case '1': { win32_change_key_state(&game_input->player1.aux1, pressed, released); } break;
        case '2': { win32_change_key_state(&game_input->player1.aux2, pressed, released); } break;
        case '3': { win32_change_key_state(&game_input->player1.aux3, pressed, released); } break;
        case '4': { win32_change_key_state(&game_input->player1.aux4, pressed, released); } break;
        case '5': { win32_change_key_state(&game_input->player1.aux5, pressed, released); } break;
        case '6': { win32_change_key_state(&game_input->player1.aux6, pressed, released); } break;
        case '7': { win32_change_key_state(&game_input->player1.aux7, pressed, released); } break;
        case '8': { win32_change_key_state(&game_input->player1.aux8, pressed, released); } break;
        case '9': { win32_change_key_state(&game_input->player1.aux9, pressed, released); } break;
      }
      
    } break;
    
    case WM_CLOSE: {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY: {
      global_state.is_running = FALSE;
    } break;
    
    default: {
      result = DefWindowProcW(window, msg, wparam, lparam);
    }
  }
  return result;
}

INTERNAL void CALLBACK win32_fiber_message(void) {
  LOCAL MSG msg;
  
  for (;;) {
    while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
    SwitchToFiber(global_state.fiber_main);
  }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show) {
  WNDCLASSEXW window_class = {0};
  HWND window;
  
  global_state.is_cursor_visible = TRUE;
  
  /* Setting up fibers */
  global_state.fiber_main = ConvertThreadToFiber(0);
  ASSERT(global_state.fiber_main != 0, L"Hey, no 'fiber_main'? No main loop.");
  global_state.fiber_message = CreateFiber(0, CAST(LPFIBER_START_ROUTINE) win32_fiber_message, 0);
  ASSERT(global_state.fiber_message != 0, L"Hey, no 'fiber_message'? No message loop.");
  
  /* Getting the executable file path */
  {
    int i, last_backslash_index;
    wchar_t path[MAX_PATH] = {0};
    DWORD result;
    
    result = GetModuleFileNameW(0, path, ARRAY_COUNT(path));
    ASSERT(result != 0, L"Couldn't ge the executable file path using 'GetModuleFileNameW(...)'!");
    last_backslash_index = 0;
    for (i = 0; (i < ARRAY_COUNT(path)) && (path[i] != '\0'); ++i) {
      if (path[i] == '\\') {
        last_backslash_index = i;
      }
    }
    if (ARRAY_COUNT(global_state.root_path) != ARRAY_COUNT(path)) {
      ASSERT(0, L"Size of 'global_path_to_exe_root' should be the same as 'path'!");
      return 1;
    }
    for (i = 0; i < last_backslash_index; ++i) {
      global_state.root_path[i] = path[i];
    }
    global_state.root_path[i] = '\0';
  }
  
  window_class.cbSize = sizeof(window_class);
  window_class.style = (CS_VREDRAW | CS_HREDRAW);
  window_class.hInstance = instance;
  /* window_class.hbrBackground = CreateSolidBrush(RGB(255, 0, 255)); */
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
    window_styles = WIN32_MAIN_WINDOW_STYLES;
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
    global_state.is_window_topmost = FALSE;
    if (global_state.is_window_topmost) {
      window = CreateWindowExW((WS_EX_TOPMOST | WS_EX_LAYERED), window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    } else {
      window = CreateWindowExW(0, window_class.lpszClassName, L"Game Window", window_styles, window_x, window_y, window_width, window_height, 0, 0, instance, 0);
    }
    ASSERT(window != 0, L"Invalid main window handle - Window couldn't be created by Windows!");
    SetWindowLongW(window, GWL_STYLE, GetWindowLongW(window, GWL_STYLE) & WIN32_MAIN_WINDOW_REMOVED_STYLES);
  }
  
  /* Game window message loop */
  {
    MSG window_msg = {0};
    GameMemory game_memory = {0};
    GameBackBuffer *game_back_buffer;
    GameInput *game_input;
    GameState *game_state;
    Win32GameCode game_code = {0};
    /* @Cleanup: Win32 layer */
    LARGE_INTEGER current_perf_counter = {0}, last_perf_counter = {0}, perf_frequency = {0};
    U32 sleep_granularity;
    float raw_elapsed_ms, cooked_elapsed_ms;
    float dt; /* this is actually 'last_frame_dt', in seconds - time elapsed in seconds since last frame */
    wchar_t game_dll_full_path[MAX_PATH] = {0}, game_temp_dll_full_path[MAX_PATH] = {0}, lock_pdb_full_path[MAX_PATH] = {0};
    S32 monitor_refresh_rate;
    
    /* Asks for memory to be used by the game */
    {
      game_memory.max_size = MEGABYTE(50);
      game_memory.address = VirtualAlloc(0, game_memory.max_size, (MEM_RESERVE | MEM_COMMIT), PAGE_READWRITE);
      game_memory.platform_free_entire_file = win32_debug_free_entire_file;
      game_memory.platform_read_entire_file = win32_debug_read_entire_file;
      game_memory.platform_write_entire_file = win32_debug_write_entire_file;
      
      /* pushing 'GameState' to game memory */
      {
        U64 game_state_size;
        
        ASSERT(game_memory.current_size == 0, L"'game_state' should be the first in 'game_memory'!"); /* @IMPORTANT: This is needed because in the game I cast the game_state pointer to the base address of the game memory */
        game_state_size = sizeof(GameState);
        game_state = CAST(GameState *) game_memory_push(&game_memory, game_state_size);
      }
      
      /* pushing 'GameBackBuffer' to game memory */
      {
        U64 game_back_buffer_size;
        
        game_back_buffer_size = sizeof(GameBackBuffer);
        game_back_buffer = CAST(GameBackBuffer *) game_memory_push(&game_memory, game_back_buffer_size);
      }
      
      /* pushing 'GameInput' to game memory */
      {
        U64 game_input_size;
        
        game_input_size = sizeof(GameInput);
        game_input = CAST(GameInput *) game_memory_push(&game_memory, game_input_size);
      }
      
    }
    
    /* Pass 'game_input' to main window callback */
    SetWindowLongPtrW(window, GWLP_USERDATA, CAST(LONG_PTR) game_input);
    
    /* Get the monitor refresh rate */
    {
      HDC window_dc;
      
      window_dc = GetDC(window);
      monitor_refresh_rate = GetDeviceCaps(window_dc, VREFRESH); /* NOTE: User's primary monitor */
      ASSERT((monitor_refresh_rate >= 30), L"Your primary monitor refresh rate is less than 30Hz, you wouldn't be able to experience the game well, so the application will be closed!");
    }
    
    /* File I/O TEST */
    {
      DWORD data[] = {0, 2, 4, 6, 8, 10};
      DWORD data_size;
      S32 i;
      
      data_size = ARRAY_COUNT(data);
      if (win32_debug_write_entire_file(L"random_data.txt", data_size, data)) {
        i = 1;
      }
    }
    
    win32_build_root_path_for_file(game_dll_full_path, ARRAY_COUNT(game_dll_full_path), L"pong.dll");
    win32_build_root_path_for_file(game_temp_dll_full_path, ARRAY_COUNT(game_temp_dll_full_path), L"pong_temp.dll");
    win32_build_root_path_for_file(lock_pdb_full_path, ARRAY_COUNT(lock_pdb_full_path), L"lock.tmp");
    
    game_code = win32_load_game_code(game_dll_full_path, game_temp_dll_full_path, lock_pdb_full_path);
    
    /* NOTE: Trying to apply high resolution Windows timers for precise sleeping */
    {
      TIMECAPS machine_timing_caps;
      
      /* @IMPORTANT: Actually this is fine if it fails, in the future just set to 1 by default, but for now I'll do this way in debug mode. */
      if (timeGetDevCaps(&machine_timing_caps, sizeof(machine_timing_caps)) != MMSYSERR_NOERROR) {
        ASSERT(0, L"Couldn't get machine timing capabilities using 'timeGetDevCaps(...)'!");
        return 1;
      }
      if (timeBeginPeriod(MIN(machine_timing_caps.wPeriodMin, machine_timing_caps.wPeriodMax)) != TIMERR_NOERROR) {
        ASSERT(0, L"Couldn't set the Windows' timer resoltuion using 'timeBeginPeriod(...)'!");
        return 1;
      }
      sleep_granularity = MIN(machine_timing_caps.wPeriodMin, machine_timing_caps.wPeriodMax);
    }
    QueryPerformanceFrequency(&perf_frequency);
    QueryPerformanceCounter(&last_perf_counter);
    dt = 0.0f;
    global_state.is_running = TRUE;
    while (global_state.is_running) {
      /* NOTE: Reload game code, if necessary */
      {
#if defined(WIN32_DEBUG)
        FILETIME current_dll_file_time = {0};
        WIN32_FILE_ATTRIBUTE_DATA dll_attributes = {0};
        DWORD result;
        
        result = GetFileAttributesExW(game_dll_full_path, GetFileExInfoStandard, &dll_attributes);
        ASSERT(result != 0, L"Couldn't get the game dll file attributes!");
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
      
      /* NOTE: Clearing controller input for new samples
-* This is needed in this case because the 'released' state
-* of the keyboard is not like the 'pressed' state. Anyway,
-* Windows stuff? */
      {
        int i;
        
        for (i = 0; i < ARRAY_COUNT(game_input->player1.buttons); ++i) {
          game_input->player1.buttons[i].released = FALSE;
        }
      }
      
      /* NOTE: Input - This is needed in order for the order of execution of fibers to work */
      SwitchToFiber(global_state.fiber_message);
      
      /* NOTE: Input */
      while (PeekMessageW(&window_msg, 0, 0, 0, PM_REMOVE)) {
        switch (window_msg.message) {
          
          
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
        
        game_back_buffer->width = global_state.back_buffer.width;
        game_back_buffer->height = global_state.back_buffer.height;
        game_back_buffer->bytes_per_pixel = WIN32_BACK_BUFFER_BYTES_PER_PIXEL;
        game_back_buffer->stride = game_back_buffer->width * game_back_buffer->bytes_per_pixel;
        game_back_buffer->memory = global_state.back_buffer.memory;
        game_input->dt = dt;
        game_code.update_and_render(game_back_buffer, game_input, &game_memory);
        
        /* Display the 'back_buffer' in window */
        {
          window_dc = GetDC(window); /* NOTE: Maybe this isn't necessary at all, since I'll do the rendering myself, I don't really need to release the DC everyframe. - @IMPORTANT: Investigate. */
          /* NOTE: BitBlt is faster than SctretchDIBits, maybe, in the future, change to BitBlt and do a bitmap resize by hand for the 'front buffer'? */
          StretchDIBits(window_dc, global_state.front_buffer_xoffset, global_state.front_buffer_yoffset, global_state.front_buffer_width, global_state.front_buffer_height, 0, 0, game_back_buffer->width, game_back_buffer->height, game_back_buffer->memory, &global_state.back_buffer.bmp_info, DIB_RGB_COLORS, SRCCOPY);
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
          
          QueryPerformanceCounter(&current_perf_counter);
          
          perf_counter_diff.QuadPart = current_perf_counter.QuadPart - last_perf_counter.QuadPart; /* counts elapsed */
          global_random_seed = CAST(U32) perf_counter_diff.QuadPart;
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
            QueryPerformanceCounter(&current_perf_counter);
            elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
            elapsed_to_desired_ms_diff = (desired_ms_per_frame - elapsed_ms);
            if (elapsed_ms < desired_ms_per_frame) {
              Sleep(CAST(S32) elapsed_to_desired_ms_diff);
              
              QueryPerformanceCounter(&current_perf_counter);
              elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
              elapsed_to_desired_ms_diff = (desired_ms_per_frame - elapsed_ms);
              ASSERT(elapsed_to_desired_ms_diff < desired_ms_per_frame, L"Game loop slept for too long, that's bad!");
              while (elapsed_ms < desired_ms_per_frame) {
                QueryPerformanceCounter(&current_perf_counter);
                elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / (F32) perf_frequency.QuadPart;
              }
            } else {
#if 0
              ASSERT(elapsed_ms < desired_ms_per_frame, L"Missed frame rate!");
#endif
            }
            cooked_elapsed_ms = ((current_perf_counter.QuadPart - last_perf_counter.QuadPart) * 1000.0f) / perf_frequency.QuadPart;
            dt = CAST(float) (current_perf_counter.QuadPart - last_perf_counter.QuadPart) / CAST(float) (perf_frequency.QuadPart); /* 'delta time' should be used when we want something to NOT BE frame rate dependent */
            last_perf_counter = current_perf_counter; /* NOTE: This should be here, right after the while loop to reach target ms per frame */
            
            /* NOTE: Debug 'time' print in the Visual Studio debugger */
#if 1
            {
              LOCAL int last_print_ticks = 0;
              
              if (GetTickCount() - last_print_ticks > 500) {
                last_print_ticks = GetTickCount();
                win32_debug_print(L"Raw MS: %fms\tCooked MS: %fms\n", raw_elapsed_ms, cooked_elapsed_ms);
              }
            }
#endif
          }
        }
      }
    }
  }
  return 0;
}