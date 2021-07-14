#include "InputManager.h"
#include <memory>
//#include <stdexcept>

struct InputManager::Data
{
	Pos mouse_pos;
	bool mouse_buttons[3] = {};
	bool keys[256] = {};
};

Pos InputManager::getMousePos()
{
	return data->mouse_pos;
}

bool InputManager::mouseBtnDown(size_t btn)
{
	return  data->mouse_buttons[btn];
}

bool InputManager::keyDown(char key)
{
	return  data->keys[key];
}

void InputManager::setKey(size_t key, bool down)
{
	data->keys[key] = down;
}

void InputManager::setMouseBtn(size_t button, bool down)
{
	data->mouse_buttons[button] = down;
}

void InputManager::setMousePos(int x, int y)
{
	std::get<0>(data->mouse_pos) = x;
	std::get<1>(data->mouse_pos) = y;
}

InputManager& InputManager::getInstance()
{
	static std::unique_ptr<Data> instance = std::make_unique<Data>();
	return reinterpret_cast<InputManager&>(std::move(instance));
}

bool InputManager::HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		InputManager::getInstance().setKey(wParam, true);
		return true;
	}
	case WM_KEYUP:
	{
		InputManager::getInstance().setKey(wParam, false);
		return true;
	}
	case WM_MOUSEMOVE:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));

		TRACKMOUSEEVENT track_mouse_event{};
		track_mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
		track_mouse_event.dwFlags = TME_LEAVE;
		track_mouse_event.dwHoverTime = HOVER_DEFAULT;
		track_mouse_event.hwndTrack = hwnd;
		TrackMouseEvent(&track_mouse_event);
		return true;
	}
	case WM_MOUSEWHEEL:
	{
		return true;
	}
	case WM_MOUSELEAVE:
	{
		InputManager::getInstance().setMouseBtn(0, false);
		InputManager::getInstance().setMouseBtn(1, false);
		InputManager::getInstance().setMouseBtn(2, false);
		return true;
	}
	case WM_LBUTTONDOWN:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(0, true);
		return true;
	}
	case WM_RBUTTONDOWN:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(2, true);
		return true;
	}
	case WM_LBUTTONUP:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(0, false);
		return true;
	}
	case WM_RBUTTONUP:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(2, false);
		return true;
	}
	case WM_MBUTTONDOWN:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(1, true);
		return true;
	}
	case WM_MBUTTONUP:
	{
		InputManager::getInstance().setMousePos(LOWORD(lParam), HIWORD(lParam));
		InputManager::getInstance().setMouseBtn(1, false);
		return true;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return true;
	}
	}
	return false;
}
