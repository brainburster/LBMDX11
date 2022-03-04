#include "LBM_Multicomponent.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	auto wnd = std::make_shared<DX11_Wnd>(hInstance);
	wnd->Size(1024, 1024)
		.WndClassName(L"Cls_LBM_Multicomponent")
		.WndName(L"×ó¼üË®£¬ÓÒ¼üÓÍ£¬ÖÐ¼üÇ½£¬shift¼üÏðÆ¤£¬¿Õ¸ñÔÝÍ£, AÏÔÊ¾¿ÕÆø, SËÙ¶È£¬VÎÐÁ¿£¬FÊÜÁ¦")
		.RemoveWndStyle(WS_MAXIMIZEBOX)
		.Init()
		.AddWndProc(WM_CLOSE, [&wnd](auto, auto) {
		wnd->Abort();
		return true;
			});

	LBM_Multicomponent lbm_program = { wnd };
	lbm_program.run();
	return 0;
}
