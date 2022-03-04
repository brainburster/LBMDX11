#include "Blizzard.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

#ifdef _DEBUG
int main()
{
	HINSTANCE hinst = GetModuleHandle(0);
	auto wnd = std::make_shared<DX11_Wnd>(hinst);
	wnd->Size(800, 600)
		.WndClassName(L"Cls_LBM_Multicomponent")
		.WndName(L"±©·çÑ©")
		.RemoveWndStyle(WS_MAXIMIZEBOX)
		.Init()
		.AddWndProc(WM_CLOSE, [&wnd](auto, auto) {
		wnd->Abort();
		return true;
			});
	Blizzard lbm_program = { wnd };
	lbm_program.run();
	return 0;
}
#else
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	auto wnd = std::make_shared<DX11_Wnd>(hInstance);
	wnd->Size(800, 600)
		.WndClassName(L"Cls_LBM_Multicomponent")
		.WndName(L"±©·çÑ©")
		.RemoveWndStyle(WS_MAXIMIZEBOX)
		.Init()
		.AddWndProc(WM_CLOSE, [&wnd](auto, auto) {
		wnd->Abort();
		return true;
			});
	Blizzard lbm_program = { wnd };
	lbm_program.run();
	return 0;
}
#endif // _DEBUG
