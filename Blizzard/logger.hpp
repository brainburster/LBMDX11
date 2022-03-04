#pragma once
#include <iostream>
#include <windows.h>

struct Logger
{
	template<typename T>
	static void log(T&& info)
	{
#ifdef _DEBUG
		std::cout << "log: " << info << '\n';
#endif // _DEBUG
	}
	template<typename T>
	static void error(T&& info)
	{
#ifdef _DEBUG
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
		std::cout << "error: " << info << '\n';
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#endif // _DEBUG
	}
};
