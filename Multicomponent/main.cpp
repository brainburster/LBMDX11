#include "LBM_Multicomponent.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPreInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	auto wnd = std::make_shared<DX11_Wnd>(hInstance);
	wnd->Size(1024, 1024)
		.WndClassName(L"Cls_LBM_Multicomponent")
		.WndName(L"左键水，右键油，中键墙，shift键橡皮，空格暂停, A显示空气, S速度，V涡量，F受力")
		.RemoveWndStyle(WS_MAXIMIZEBOX)
		.Init();
	LBM_Multicomponent lbm_program = { wnd };
	lbm_program.run();
	return 0;
}
