#pragma once

#include <windows.h>
#include <string>
#include <functional>
#include <map>
#include <tuple>

#define $_str(x) #x
#define $str(x) $_str(x)
#define $get_location_str() TEXT("\nfile:"$str(__FILE__)"\nfunction:" $str(__FUNCTION__)"\nline:"$str(__LINE__))

class Wnd_Base
{
public:
	using MSG_ID = std::tuple<HWND, UINT>;
	using MSG_Handler = std::function<bool(WPARAM, LPARAM)>;
	using MSG_MAP = std::map<MSG_ID, MSG_Handler>;
	using string = std::wstring;

	static void PeekMsg()
	{
		while (PeekMessage(&Msg(), NULL, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&Msg());
			DispatchMessage(&Msg());
		}
	}
	static BOOL GetMsg()
	{
		if (BOOL bRet = GetMessage(&Msg(), NULL, 0, 0))
		{
			if (bRet == -1)
			{
				return -1;
			}

			TranslateMessage(&Msg());
			DispatchMessage(&Msg());

			return 1;
		}
		return 0;
	}
	static MSG& Msg()
	{
		static MSG msg = { };
		return msg;
	}

	static MSG_MAP& MsgMap()
	{
		static MSG_MAP msg_map;
		return msg_map;
	}

	static bool& app_should_close() noexcept
	{
		static bool bClose = false;
		return bClose;
	}
	;
	static void Abort() noexcept
	{
		app_should_close() = true;
	}

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_DESTROY)
		{
			Wnd_Base::Abort();
		}
		const MSG_ID msg_id = { hWnd, message };
		if (MsgMap().find(msg_id) != MsgMap().end() && MsgMap().at(msg_id)(wParam, lParam))
		{
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	void AdjustWnd()
	{
		const int cxScreen = GetSystemMetrics(SM_CXSCREEN);
		const int cyScreen = GetSystemMetrics(SM_CYSCREEN);

		RECT rect = { 0 };
		rect.left = (cxScreen - m_width) / 2;
		rect.top = (cyScreen - m_height) / 2;
		rect.right = (cxScreen + m_width) / 2;
		rect.bottom = (cyScreen + m_height) / 2;

		AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);
		MoveWindow(m_hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
		ShowWindow(m_hwnd, SW_SHOW);
	}

	static void ShowLastError()
	{
		TCHAR szBuf[128]{};
		LPVOID lpMsgBuf = nullptr;
		DWORD dw = GetLastError();
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		wsprintf((LPWSTR)szBuf, TEXT("location:%s\nerror code: %d\ninfo: %s"),
			$get_location_str(),
			dw, (LPWSTR)lpMsgBuf);
		LocalFree(lpMsgBuf);
		OutputDebugString(szBuf);
		MessageBox(NULL, szBuf, TEXT("last error:"), MB_OK);
	}

	Wnd_Base& operator=(Wnd_Base&& other) noexcept {
		if (this == &other)
		{
			return *this;
		}

		memcpy(this, &other, sizeof(Wnd_Base));
		memset(&other, 0, sizeof(Wnd_Base));
		return *this;
	};
#pragma warning(disable:26495)
	Wnd_Base(Wnd_Base&& other) noexcept
	{
		memcpy(this, &other, sizeof(Wnd_Base));
		memset(&other, 0, sizeof(Wnd_Base));
	};
#pragma warning(default:26495)

	Wnd_Base(HINSTANCE hinst) :
		m_hinst{ hinst },
		m_hwnd{ NULL },
		m_width{ 0 },
		m_height{ 0 },
		m_wnd_style{ WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME },
		m_wnd_name{ L"default_wnd_name" },
		m_wnd_class_name{ L"default_wnd_class_name" }
	{
	}

	UINT getWidth()
	{
		return m_width;
	}

	UINT getHeight()
	{
		return m_height;
	}

protected:
	HWND m_hwnd;
	HINSTANCE m_hinst;
	UINT m_width;
	UINT m_height;
	DWORD m_wnd_style;
	string m_wnd_name;
	string m_wnd_class_name;

	//Wnd_Base() = default;
private:
	Wnd_Base(const Wnd_Base&) = delete;
	Wnd_Base& operator=(const Wnd_Base&) = delete;
};

class Wnd_Default;

template<typename Concrete = Wnd_Default>
class Wnd : public Wnd_Base
{
public:
	using Wnd_Base::MSG_ID;
	using Wnd_Base::MSG_Handler;
	using Wnd_Base::MSG_MAP;
	using Wnd_Base::string;

	HWND Hwnd() const noexcept
	{
		return m_hwnd;
	}

	HINSTANCE Hinst() const noexcept
	{
		return m_hinst;
	}

	Concrete& Size(UINT w, UINT h) noexcept
	{
		m_width = w;
		m_height = h;
		return static_cast<Concrete&>(*this);
	}
	Concrete& WndName(const string& wnd_name)
	{
		m_wnd_name = wnd_name;
		return static_cast<Concrete&>(*this);
	}
	Concrete& WndClassName(const string& wnd_class_name)
	{
		m_wnd_class_name = wnd_class_name;
		return static_cast<Concrete&>(*this);
	}
	Concrete& WndStyle(DWORD wnd_style) noexcept
	{
		m_wnd_style = wnd_style;
		return static_cast<Concrete&>(*this);
	}
	Concrete& AddWndStyle(DWORD wnd_style) noexcept
	{
		m_wnd_style |= wnd_style;
		return static_cast<Concrete&>(*this);
	}

	Concrete& RemoveWndStyle(DWORD wnd_style) noexcept
	{
		m_wnd_style &= ~wnd_style;
		return static_cast<Concrete&>(*this);
	}

	const string& WndName() const noexcept
	{
		return m_wnd_name;
	}

	const string& WndClassName() const noexcept
	{
		return m_wnd_class_name;
	}

	DWORD WndStyle() const noexcept
	{
		return m_wnd_style;
	}

	template<typename callback_t = MSG_Handler>
	Concrete& AddWndProc(UINT message, callback_t&& callback)
	{
		const MSG_ID msg_id = { m_hwnd, message };
		if (MsgMap().find(msg_id) == MsgMap().end())
		{
			MsgMap()[msg_id] = std::forward<callback_t>(callback);
		}
		else
		{
			MsgMap()[msg_id] = [prev{ std::move(MsgMap()[msg_id]) }, curr{ std::forward<callback_t>(callback) }](auto a, auto b)->bool {return prev(a, b) && curr(a, b); };
		}
		return static_cast<Concrete&>(*this);
	}

	Concrete& Init()
	{
		WNDCLASS wc = { 0 };
		wc.lpfnWndProc = Wnd_Base::WndProc;
		wc.hInstance = m_hinst;
		wc.lpszClassName = m_wnd_class_name.c_str();
		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(NULL, m_wnd_class_name.c_str(), m_wnd_name.c_str(), m_wnd_style,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, m_hinst, NULL);

		if (!m_hwnd)
		{
			ShowLastError();
			Wnd::Abort();
		}

		AdjustWnd();

		HCURSOR hcur = LoadCursor(NULL, IDC_ARROW);
		SetCursor(hcur);

		return static_cast<Concrete&>(*this);
	}

	Concrete&& Move()
	{
		return static_cast<Concrete&&>(*this);
	}

	virtual ~Wnd()
	{
		if (m_hwnd)
		{
			::SendMessage(m_hwnd, WM_QUIT, 0, 0);
		}
		if (m_wnd_class_name.c_str())
		{
			UnregisterClass(m_wnd_class_name.c_str(), m_hinst);
		}
	}

	Wnd& operator=(Wnd&& other) noexcept {
		if (this == &other)
		{
			return *this;
		}

		memcpy(this, &other, sizeof(Wnd));
		memset(&other, 0, sizeof(Wnd));
		return *this;
	};

#pragma warning(disable:26495)
	Wnd(Wnd&& other) noexcept
	{
		memcpy(this, &other, sizeof(Wnd));
		memset(&other, 0, sizeof(Wnd));
	};
#pragma warning(default:26495)

	Wnd(HINSTANCE hinst) :Wnd_Base{ hinst }
	{
	}
protected:
	//Wnd() = default;
private:
	Wnd(const Wnd&) = delete;
	Wnd& operator=(const Wnd&) = delete;
};

class Wnd_Default : public Wnd<Wnd_Default> {};
