#include <windows.h>
#include <d3d11.h>
#include <tuple>
#include <stdexcept>

#pragma comment(lib,"d3d11.lib")

enum class EWndSize : int
{
	width = 800,
	height = 600
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		//FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND CreateWnd(HINSTANCE hinst)
{
	// Register the window class.
	const wchar_t CLASS_NAME[] = L"DX11_LBM_WIN_CLASS";
	const wchar_t TITLE_NAME[] = L"DX11_LBM";

	WNDCLASS wc = { 0 };

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hinst;
	wc.lpszClassName = CLASS_NAME;

	RegisterClass(&wc);

	// Create the window.

	HWND hwnd = CreateWindowEx(
		0,                              // Optional window styles.
		CLASS_NAME,                     // Window class
		TITLE_NAME,                     // Window text
		WS_OVERLAPPEDWINDOW,            // Window style

		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

		NULL,       // Parent window
		NULL,       // Menu
		hinst,  // Instance handle
		NULL        // Additional application data
	);

	if (hwnd == NULL)
	{
		throw std::runtime_error("Can't create window");
	}

	const int cxScreen = GetSystemMetrics(SM_CXSCREEN);
	const int cyScreen = GetSystemMetrics(SM_CYSCREEN);

	RECT rect;
	rect.left = (cxScreen - (int)EWndSize::width) / 2;
	rect.right = (cxScreen + (int)EWndSize::width) / 2;
	rect.top = (cyScreen - (int)EWndSize::height) / 2;
	rect.bottom = (cyScreen + (int)EWndSize::height) / 2;

	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);
	MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
	ShowWindow(hwnd, SW_SHOW);
	return hwnd;
}

auto CreateDeviceAndSwapChain(HWND hwnd) -> std::tuple<ID3D11Device*, IDXGISwapChain*, ID3D11DeviceContext*>
{
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Device* device;
	ID3D11DeviceContext* context; //immediate context
	DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	IDXGISwapChain* swap_chain;

	swap_chain_desc.BufferDesc.Width = (int)EWndSize::width;
	swap_chain_desc.BufferDesc.Height = (int)EWndSize::height;
	swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	swap_chain_desc.SampleDesc.Count = 1;
	swap_chain_desc.SampleDesc.Quality = 0;

	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 1;
	swap_chain_desc.OutputWindow = hwnd;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_chain_desc.Flags = 0;

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	HRESULT hr = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swap_chain_desc, &swap_chain, &device, &featureLevel, &context);
	if (FAILED(hr))
	{
		throw std::runtime_error("Can't create d3d11 device and swapChain. ");
	}
	if (featureLevel != D3D_FEATURE_LEVEL_11_0)
	{
		throw std::runtime_error("D3d featureLevel 11  unsupported. ");
	}

	return std::forward_as_tuple(device, swap_chain, context);
}

void update(ID3D11Device* device, IDXGISwapChain* swap_chain, ID3D11DeviceContext* context)
{
	ID3D11RenderTargetView* render_target_view;
	ID3D11Texture2D* back_buffer;
	swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(&back_buffer));
	if (!back_buffer)
	{
		throw std::runtime_error("can't get backbuffer");
	}
	device->CreateRenderTargetView(back_buffer, 0, &render_target_view);
	if (!render_target_view)
	{
		throw std::runtime_error("can't create RenderTargetView");
	}
	context->OMSetRenderTargets(1, &render_target_view, 0);
	float clear_color[4] = { 0.5f,0.1f,0.1f,0.5f };
	context->ClearRenderTargetView(render_target_view, clear_color);
	back_buffer->Release();

	swap_chain->Present(0, 0);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int nCmdSHow)
{
	HWND hwnd = CreateWnd(hInst);

	ID3D11Device* device;
	IDXGISwapChain* swap_chain;
	ID3D11DeviceContext* context;

	std::tie(device, swap_chain, context) = CreateDeviceAndSwapChain(hwnd);

	MSG msg = { 0 };

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			update(device, swap_chain, context);
		}
	}

	return msg.wParam;
}
