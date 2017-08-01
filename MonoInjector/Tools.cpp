#include "Tools.h"
#include  <fstream>

namespace MonoInjector
{
	namespace tools
	{
		DWORD GetProcessPid(const std::wstring& name) noexcept
		{
			DWORD pid = 0;

			std::vector<DWORD> found = blackbone::Process::EnumByName(name);

			if (found.size() > 0)
				pid = found.front();

			return pid;
		}

		bool IsFileExists(const std::string& fileName) noexcept
		{
			std::ifstream f(fileName);

			bool result = f.good();
			f.close();

			return result;
		}

		std::vector<char> ReadAllBytes(const std::string& fileName) noexcept
		{
			std::ifstream input(fileName, std::ios::binary);
			if (input.is_open())
			{
				std::vector<char> buffer((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
				return buffer;
			}

			return std::vector<char>();
		}
	}
}
