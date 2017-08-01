#pragma once

#include "Tools.h"

namespace MonoInjector
{
	struct AssemblyInfo
	{
		std::string file_name;
		std::string name_space;
		std::string class_name;
		std::string method_name;
		bool bUnlink;
	};

	class Injector
	{
	public:
		Injector() = default;
		~Injector() = default;

		bool InjectDll(DWORD pid, const AssemblyInfo& assembly_info);
	private:
		blackbone::Process process_;
		blackbone::ThreadPtr context_thread_;
		blackbone::ModuleDataPtr module_data_ptr_;

		NTSTATUS Attach(DWORD pid);
		intptr_t GetRootDomain();
		intptr_t GetDomain();
		intptr_t ThreadAttach(intptr_t domain);
		void SecuritySetMode(int mode);
		intptr_t ImageOpenFromDataFull(const std::vector<char>& image);
		intptr_t AssemblyLoadFromFull(intptr_t raw_image);
		intptr_t AssemblyGetImage(intptr_t assembly);
		intptr_t GetClassFromName(intptr_t image, const char* name_space, const char* class_name);
		intptr_t GetMethodFromName(intptr_t class_id, const char* method_name);
		intptr_t RuntimeInvokeMethod(intptr_t method_id);
		int ThreadDetach(intptr_t domain);
		NTSTATUS UnlinkAssembly(intptr_t domain);
	};
}
