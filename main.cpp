// minimize the amount of stuff to be included from the windows header.
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <Windows.h>

#include <string>

// ============================================================================

constexpr auto WINDOW_CLASS_NAME = "D2D-SANDBOX";

// ============================================================================

inline void fail(const std::string& description)
{
  // construct and show an informative message to the user.
  std::string str("Application has crashed because of an fatal failure.\n\n");
  str += description;
  MessageBox(nullptr, str.c_str(), "Application Error", MB_OK);

  // break here whether the debugger is currently attached.
  if (IsDebuggerPresent())
    __debugbreak();

  // kill the application.
  FatalExit(1);
}

// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================================

int main()
{
  // build and register the window class for the application.
  WNDCLASSEX windowClass = {};
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hInstance = GetModuleHandle(nullptr);
  windowClass.lpfnWndProc = WndProc;
  windowClass.lpszClassName = WINDOW_CLASS_NAME;
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  if (RegisterClassEx(&windowClass) != 0) {
    fail("RegisterClassEx failed");
  }



  // release all reserved resources.
  UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
  return 0;
}