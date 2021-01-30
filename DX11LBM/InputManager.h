#pragma once

#include <Windows.h>
#include <tuple>

using Pos = std::tuple<int, int>;

class InputManager
{
public:
	Pos getMousePos();
	bool mouseBtnDown(size_t btn);
	bool keyDown(char key);
	void setKey(size_t key, bool down);
	void setMouseBtn(size_t button, bool down);
	void setMousePos(int x, int y);
	static InputManager& getInstance();
	static InputManager& getLast();
private:
	Pos mouse_pos;
	bool mouse_buttons[3];
	bool keys[256];
};
