#include <windows.h>

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
		//throw std::runtime_error("Can't create window");
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

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int nCmdSHow)
{
	HWND hwnd = CreateWnd(hInst);

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
			//...
		}
	}

	return msg.wParam;
}
