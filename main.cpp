// minimize the amount of stuff to be included from the windows header.
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <Windows.h>
#include <wrl.h>
#include <comdef.h>

#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>

#include <cassert>
#include <string>

using namespace Microsoft::WRL;

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")

// ============================================================================

constexpr auto WINDOW_CLASS_NAME = "D2D-SANDBOX";
constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

// ============================================================================

struct D3DContext
{
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> deviceCtx;
};

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

inline void throwOnFail(HRESULT hr)
{
  if (FAILED(hr)) {
    throw _com_error(hr);
  }
}

// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
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
    if (IsWindow(gHwnd) && DestroyWindow(gHwnd) == 0) {
      fail("DestroyWindow failed");
    }
  });
}

// ============================================================================
// Create a new Direct2D factory object.
//
// Direct2D handles factory objects as the root of building a new application.
// These objects are used to build base resources that can be used to form the
// final application structure and functionality.
//
// The global Direct2D debugging can be enabled by passing the factory options
// to construction method by selecting any of the following values.
//   D2D1_DEBUG_LEVEL_NONE..........To disable debug messages (production).
//   D2D1_DEBUG_LEVEL_ERROR.........To generate only messages about errors.
//   D2D1_DEBUG_LEVEL_WARNING.......To generate messages of errors and warn.
//   D2D1_DEBUG_LEVEL_INFORMATION...To generate errors, warns and trace info.
//
// Direct2D allows factories to be created either for a multithreaded or single
// threaded environment. This selection is passed to construction method.
//   D2D1_FACTORY_TYPE_SINGLE_THREADED...To build for a single thread usage.
//   D2D1_FACTORY_TYPE_MULTI_THREADED....To build for multithreaded usage.
//
// The way how Direct2D handles multithreaded items is based on serialisation,
// where single threaded factories is not required to serialize incoming calls.
// Multithreaded factories can be used by many threads, as calls are serialised
// before they are executed, while also allowing resources to be shared between
// the application threads. The definition which method to select is based how
// the application is structured and how Direct2D components are being used.
// ============================================================================
ComPtr<ID2D1Factory> createD2DFactory()
{
  // create creation options for the Direct2D factory item.
  D2D1_FACTORY_OPTIONS options;
  #ifdef _DEBUG
  options.debugLevel = D2D1_DEBUG_LEVEL_WARNING;
  #endif

  // construct a new Direct2D factory to build Direct2D resources.
  ComPtr<ID2D1Factory> factory;
  throwOnFail(D2D1CreateFactory(
    D2D1_FACTORY_TYPE_SINGLE_THREADED,
    options,
    factory.GetAddressOf()
  ));

  // return the new factory.
  return factory;
}

// ============================================================================
// Create a new Direct3D device and device context objects.
//
// Direct2D requires Direct3D device to construct a device context object. This
// device is associated with the Direct2D device when creating the context. Note
// that this construction is similar to typical clear Direct3D but with a twist.
// 
// Direct3D does allow applications to create devices which may emulate how the
// real supported device would work. Here's the list of possible driver types.
//   D3D_DRIVER_TYPE_NULL........A D3D driver without rendering support.
//   D3D_DRIVER_TYPE_UNKNOWN.....The driver type is unknown.
//   D3D_DRIVER_TYPE_SOFTWARE....Software emulation.
//   D3D_DRIVER_TYPE_REFERENCE...Software emulation with full D3D support.
//   D3D_DRIVER_TYPE_HARDWARE....A hardware driver which implements Direct3D.
//   D3D_DRIVER_TYPE_WARP........High performance software rasterizer.
//
// Direct3D device must be initialised with an additional color channel support
// so it can be correctly used with the Direct2D. This is because Direct3D uses
// different channel ordering than Direct2D by default. Application should set
// the D3D11_CREATE_DEVICE_BGRA_SUPPORT value as a creation flag. Here's the
// full list of creation flags that can be used with Direct3D device creation.
//   D3D11_CREATE_DEVICE_BGRA_SUPPORT..........
//   D3D11_CREATE_DEVICE_DEBUG.................Enable the D3D debug layer.
//   D3D11_CREATE_DEVICE_DEBUGGABLE............Enable shader debugger.
//   D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT...Disable GPU timeout detection.
//   D3D11_CREATE_DEVICE_PREVENT_ALTERING_*....Prevent the use of D3D debugger.
//   D3D11_CREATE_DEVICE_PREVENT_INTERNAL_*....Disable WARP multithreading.
//   D3D11_CREATE_DEVICE_SINGLETHREADED........Optimize for singlethread usage.
//   D3D11_CREATE_DEVICE_SWITCH_TO_REF.........[Not supported]
//   D3D11_CREATE_DEVICE_VIDEO_SUPPORT.........Fail if non-WDDM 1.2 compatible.
// ============================================================================
D3DContext createD3DContext()
{
  // specify the desired additional behavior how the device will be created.
  UINT flags = 0;
  flags |= D3D11_CREATE_DEVICE_BGRA_SUPPORT; // for Direct2D compatibility
  flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
  #ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
  #endif

  // specify the feature levels we want to support (ordering matters!).
  D3D_FEATURE_LEVEL featureLevels[] = 
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3,
    D3D_FEATURE_LEVEL_9_2,
    D3D_FEATURE_LEVEL_9_1
  };

  // construct a new Direct3D device and a device context.
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> deviceContext;
  throwOnFail(D3D11CreateDevice(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    0,
    flags,
    featureLevels,
    ARRAYSIZE(featureLevels),
    D3D11_SDK_VERSION,
    device.GetAddressOf(),
    nullptr,
    deviceContext.GetAddressOf()
  ));

  // return a Direct3D context containing the created objects.
  return { device, deviceContext };
}

// ============================================================================

int main()
{
  registerWindowClass();
  createWindow();

  // set the window visible.
  ShowWindow(gHwnd, SW_SHOW);
  UpdateWindow(gHwnd);

  // initialise Direct2D framework.
  auto factory = createD2DFactory();
  auto d3dCtx = createD3DContext();

  // start the main loop of the application.
  MSG msg = {};
  while (msg.message != WM_QUIT) {
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return 0;
}