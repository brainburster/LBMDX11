#pragma once

#include <tuple>
#include <memory>

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
	struct Data;
	std::unique_ptr<Data> data;
};
