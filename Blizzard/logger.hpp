#pragma once
#include <iostream>
#include <windows.h>

struct Logger
{
	template<typename T>
	static void log(T&& info)
	{
		std::cout << "log: " << info << '\n';
	}
	template<typename T>
	static void error(T&& info)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		std::cout << "error: " << info << '\n';
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	}
};
