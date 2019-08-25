#pragma once
#include <exception>
#include <string>
#include <sstream>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

class win32_exception : std::exception
{
	std::string message_;
public:
	win32_exception(const DWORD error_code, const std::string_view message)
	{
		std::stringstream ss;
		ss << message;
		ss << "\nError code: " << error_code << '\n';

		message_ = ss.str();
	}

	[[nodiscard]] char const* what() const override
	{
		return message_.c_str();
	}
};

