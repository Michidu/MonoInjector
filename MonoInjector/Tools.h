#pragma once

#include <Windows.h>
#include "../BlackBone/src/BlackBone/Process/Process.h"

namespace MonoInjector
{
	namespace tools
	{
		DWORD GetProcessPid(const std::wstring& name) noexcept;
		bool IsFileExists(const std::string& file_name) noexcept;
		std::vector<char> ReadAllBytes(const std::string& file_name) noexcept;
	}
}
