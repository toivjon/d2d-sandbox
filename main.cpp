// minimize the amount of stuff to be included from the windows header.
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <Windows.h>

#include <cassert>
#include <string>

// ============================================================================

constexpr auto WINDOW_CLASS_NAME = "D2D-SANDBOX";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;
 
// ============================================================================

HWND gHwnd = nullptr;

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

void registerWindowClass()
{
  // build a new empty window class descriptor.
  WNDCLASSEX windowClass = {};

  // configure the class desriptor with desired definitions.
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
  windowClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
  windowClass.hInstance = GetModuleHandle(nullptr);
  windowClass.lpfnWndProc = WndProc;
  windowClass.lpszClassName = WINDOW_CLASS_NAME;
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  
  // register the descriptor so we can start using the new window class.
  if (RegisterClassEx(&windowClass) == 0) {
    fail("RegisterClassEx failed");
  }

  // specify that we unregister the window class when we exit the application.
  atexit([](){
    if (UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr)) == 0) {
      fail("UnregisterClass failed");
    }
  });
}

// ============================================================================

void createWindow()
{
  assert(gHwnd == nullptr);

  // construct the main window for the application.
  gHwnd = CreateWindowEx(
    WS_EX_CLIENTEDGE,
    WINDOW_CLASS_NAME,
    "D2D Sanbox",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    WINDOW_WIDTH,
    WINDOW_HEIGHT,
    nullptr,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr);

  // check how the operation succeeded.
  if (gHwnd == nullptr) {
    fail("CreateWindowEx failed");
  }

  // specify that we want to destroy the window when we exit the application.
  atexit([]() {
    if (DestroyWindow(gHwnd) == 0) {
      fail("DestroyWindow failed");
    }
  });
}

// ============================================================================

int main()
{
  registerWindowClass();
  createWindow();

  return 0;
}