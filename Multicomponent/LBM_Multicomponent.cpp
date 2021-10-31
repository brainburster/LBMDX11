#include "LBM_Multicomponent.h"

void LBM_Multicomponent::add_control_point(XMFLOAT2 pos, XMFLOAT2 data)
{
	if (last_control_point.pos.x >= 0 && data.y == last_control_point.data.y)
	{
		XMVECTOR pos0 = XMLoadFloat2(&last_control_point.pos);
		XMVECTOR pos1 = XMLoadFloat2(&pos);
		XMVECTOR direction = pos1 - pos0;
		XMVECTOR len = XMVector2Length(direction);
		direction = direction / len;
		const float r = data.x;
		const float length = XMVectorGetX(len);
		for (float d = r; d < length; d += r)
		{
			XMVECTOR pos2 = pos0 + XMVectorSet(d, d, 0, 0) * direction;
			XMFLOAT2 pos_float2 = {};
			XMStoreFloat2(&pos_float2, pos2);
			control_points.push_back({ pos_float2 , data });
		}
	}

	control_points.push_back({ pos , data });
	last_control_point = { pos , data };
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
			add_control_point(pos, { 10.f,3.f });
		}

		TRACKMOUSEEVENT track_mouse_event{};
		track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
		track_mouse_event.dwFlags = TME_LEAVE;
		track_mouse_event.dwHoverTime = HOVER_DEFAULT;
		track_mouse_event.hwndTrack = dx11_wnd->Hwnd();
		TrackMouseEvent(&track_mouse_event);
		return true;
	};

	const auto onmousebtnupormouseleave = [&](WPARAM wparam, LPARAM lparam) {
		last_control_point = { {-1.f,-1.f},{-1.f,-1.f} };
		return true;
	};

	dx11_wnd->AddWndProc(WM_MOUSEMOVE, onmousemove);
	dx11_wnd->AddWndProc(WM_MOUSELEAVE, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_LBUTTONUP, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_RBUTTONUP, onmousebtnupormouseleave);
	dx11_wnd->AddWndProc(WM_MBUTTONUP, onmousebtnupormouseleave);
}
