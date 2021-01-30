#include "InputManager.h"
#include <stdexcept>

Pos InputManager::getMousePos()
{
	return mouse_pos;
}

bool InputManager::mouseBtnDown(size_t btn)
{
	return mouse_buttons[btn];
}

bool InputManager::keyDown(char key)
{
	return keys[key];
}

void InputManager::setKey(size_t key, bool down)
{
	keys[key] = down;
}

void InputManager::setMouseBtn(size_t button, bool down)
{
	mouse_buttons[button] = down;
}

void InputManager::setMousePos(int x, int y)
{
	std::get<0>(mouse_pos) = x;
	std::get<1>(mouse_pos) = y;
}

InputManager& InputManager::getInstance()
{
	static InputManager instance = {};
	return *&instance;
}
