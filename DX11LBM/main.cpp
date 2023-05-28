#include "InputManager.h"
#include "LBM.hpp"
#include "chaincall.hpp"
#include "header.hpp"
#include <d3d11.h>
#include <functional>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <windows.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  if (InputManager::getInstance().handleInput(hwnd, uMsg, wParam, lParam)) {
    return 0;
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND createWnd(HINSTANCE hinst) {
  // Register the window class.

  WNDCLASS wc = {0};

  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hinst;
  wc.lpszClassName = Setting::cls_name;

  RegisterClass(&wc);

  // Create the window.

  HWND hwnd =
      CreateWindowEx(0,                 // Optional window styles.
                     Setting::cls_name, // Window class
                     Setting::wnd_name, // Window text
                     WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, // Window style

                     // Size and position
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                     NULL,  // Parent window
                     NULL,  // Menu
                     hinst, // Instance handle
                     NULL   // Additional application data
      );

  if (hwnd == NULL) {
    throw std::runtime_error("Failed to create window");
  }

  const int cxScreen = GetSystemMetrics(SM_CXSCREEN);
  const int cyScreen = GetSystemMetrics(SM_CYSCREEN);

  RECT rect{};
  rect.left = (cxScreen - Setting::width) / 2;
  rect.right = (cxScreen + Setting::width) / 2;
  rect.top = (cyScreen - Setting::height) / 2;
  rect.bottom = (cyScreen + Setting::height) / 2;

  AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);
  MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left,
             rect.bottom - rect.top, true);
  ShowWindow(hwnd, SW_SHOW);
  SetActiveWindow(hwnd);
  HCURSOR hcur = LoadCursor(NULL, IDC_ARROW);
  SetCursor(hcur);
  return hwnd;
}

auto createDeviceAndSwapChain(HWND hwnd)
    -> std::tuple<ID3D11Device *, IDXGISwapChain *, ID3D11DeviceContext *> {
  D3D_FEATURE_LEVEL featureLevel;
  ID3D11Device *device;
  ID3D11DeviceContext *context; // immediate context
  DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
  IDXGISwapChain *swap_chain;

  swap_chain_desc.BufferDesc.Width = Setting::width;
  swap_chain_desc.BufferDesc.Height = Setting::height;
  swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
  swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
  swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_desc.BufferDesc.ScanlineOrdering =
      DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

  swap_chain_desc.SampleDesc.Count = 1;
  swap_chain_desc.SampleDesc.Quality = 0;

  swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

  swap_chain_desc.BufferCount = 1;
  swap_chain_desc.OutputWindow = hwnd;
  swap_chain_desc.Windowed = true;
  swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swap_chain_desc.Flags = 0;

  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
  };
  UINT numFeatureLevels = ARRAYSIZE(featureLevels);

  HRESULT hr = D3D11CreateDeviceAndSwapChain(
      0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, featureLevels, numFeatureLevels,
      D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &device, &featureLevel,
      &context);
  if (FAILED(hr)) {
    throw std::runtime_error("Failed to create d3d11 device and swapChain. ");
  }
  if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
    throw std::runtime_error("D3d featureLevel 11  unsupported. ");
  }

  return std::forward_as_tuple(device, swap_chain, context);
}

void msgLoop(std::function<void()> update) {
  MSG msg = {0};

  bool stop = false;
  std::thread t{[&]() {
    while (!stop) {
      update();
    }
  }};

  while (WM_QUIT != msg.message) {
    while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  stop = true;
  t.join();
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
  using namespace std::placeholders;

  LBM lbm;

  // auto init_lbm = std::bind(&LBM::init, &lbm, _1, _2, _3);
  auto init_lbm = [&](ID3D11Device *device, IDXGISwapChain *swap_chain,
                      ID3D11DeviceContext *context) {
    return lbm.init(device, swap_chain, context);
  };
  auto get_lbm_process = [&] { return std::bind(&LBM::process, &lbm); };

  auto app_process = chaincall::pipe() >> createWnd >>
                     createDeviceAndSwapChain >> init_lbm >> get_lbm_process >>
                     msgLoop;

  try {
    app_process(hInstance);
  } catch (const std::exception &e) {
    MessageBoxA(NULL, e.what(), "error", MB_OK);
  }

  return 0;
}
