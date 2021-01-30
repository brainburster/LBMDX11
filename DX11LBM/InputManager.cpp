#include "InputManager.h"
#include <memory>
#include <stdexcept>

struct InputManager::Data
{
	Pos mouse_pos;
	bool mouse_buttons[3];
	bool keys[256];
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
