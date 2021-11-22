#pragma once

struct Setting
{
	enum
	{
		width = 800,
		height = 600
	};
	static constexpr wchar_t cls_name[] = L"DX11_LBM_WIN_CLASS";
	static constexpr wchar_t wnd_name[] = L"DX11_LBM";
};
