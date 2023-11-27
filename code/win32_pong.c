#define _UNICODE
#define UNICODE

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prev_instance, PWSTR cmd_line, int cmd_show) {
  MessageBoxW(0, L"Hello Message", L"Hello, Windows!", MB_OK);
  return 0;
}