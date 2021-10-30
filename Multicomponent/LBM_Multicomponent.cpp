#include "LBM_Multicomponent.h"

void LBM_Multicomponent::add_control_point(XMFLOAT2 pos, XMFLOAT2 data)
{
	control_points.push_back({ pos , data });
}

void LBM_Multicomponent::set_input_callback()
{
	const auto onmousemove = [&](WPARAM wparam, LPARAM lparam) {
		const POINTS p = MAKEPOINTS(lparam);
		const XMFLOAT2 pos = { (float)p.x,(float)p.y };
		if (wparam & MK_LBUTTON) {
			add_control_point(pos, { 10.f,1.f });
		}
		else if (wparam & MK_RBUTTON) {
			add_control_point(pos, { 10.f,2.f });
		}
		else if (wparam & MK_MBUTTON) {
			add_control_point(pos, { 10.f,-1.f });
		}
		return true;
	};

	dx11_wnd->AddWndProc(WM_MOUSEMOVE, onmousemove);
}
