#include "Injector.h"
#include <iostream>
#include "../BlackBone/src/BlackBone/Process/RPC/RemoteFunction.hpp"

namespace MonoInjector
{
	bool Injector::InjectDll(DWORD pid, const AssemblyInfo& assembly_info)
	{
		if (Attach(pid) != STATUS_SUCCESS)
		{
			std::wcout << "Could not attach to the process" << std::endl;
			return false;
		}

		auto barrier = process_.core().native()->GetWow64Barrier().type;
		bool valid_arch = barrier == blackbone::wow_64_64 || barrier == blackbone::wow_32_32;
		if (!valid_arch)
		{
			std::wcout << "Can't call function through WOW64 barrier";
			return false;
		}

		module_data_ptr_ = process_.modules().GetModule(L"mono.dll");
		if (module_data_ptr_->baseAddress == 0)
		{
			std::wcout << "Could not find module mono.dll" << std::endl;
			return false;
		}

		if (process_.remote().CreateRPCEnvironment(blackbone::Worker_CreateNew) != STATUS_SUCCESS)
		{
			std::wcout << "Can't create RPC" << std::endl;
			return false;
		}

		context_thread_ = process_.remote().getWorker();

		auto file_data = tools::ReadAllBytes(assembly_info.file_name);

		try
		{
			std::wcout << std::endl << "Debug Info:" << std::endl << std::endl;

			intptr_t domain = GetRootDomain();
			if (domain == 0)
			{
				std::cout << std::endl << "Can't find root domain" << std::endl;
				return false;
			}

			std::wcout << "Root Domain - " << domain << std::endl;

			ThreadAttach(domain);

			SecuritySetMode(0);

			intptr_t raw_image = ImageOpenFromDataFull(file_data);
			std::wcout << "Raw Image - " << raw_image << std::endl;

			intptr_t assembly = AssemblyLoadFromFull(raw_image);
			std::wcout << "Assembly - " << assembly << std::endl;

			intptr_t image = AssemblyGetImage(assembly);
			std::wcout << "Image - " << image << std::endl;

			intptr_t class_id = GetClassFromName(image, assembly_info.name_space.c_str(), assembly_info.class_name.c_str());
			if (class_id == 0)
			{
				std::cout << std::endl << "Can't find class " << assembly_info.name_space.c_str() << "::" << assembly_info.class_name.c_str() << std::endl;
				return false;
			}

			std::wcout << "Class id - " << class_id << std::endl;

			intptr_t method = GetMethodFromName(class_id, assembly_info.method_name.c_str());
			if (method == 0)
			{
				std::cout << std::endl << "Can't find method " << assembly_info.method_name.c_str() << std::endl;
				return false;
			}

			std::wcout << "Method id - " << method << std::endl;

			if (assembly_info.bUnlink)
			{
				NTSTATUS status = UnlinkAssembly(domain);
				std::wcout << std::endl << (NT_SUCCESS(status) ? "Unlinked assembly" : "Failed to unlink assembly") << std::endl;
			}

			RuntimeInvokeMethod(method);

			//ThreadDetach(module_data_ptr_, context_thread, domain);
		}
		catch (const std::exception& exception)
		{
			std::wcout << std::endl << "Error - " << exception.what() << std::endl;
			return false;
		}

		return true;
	}

	NTSTATUS Injector::Attach(DWORD pid)
	{
		return process_.Attach(pid);
	}

	intptr_t Injector::GetRootDomain()
	{
		using mono_get_root_domain = intptr_t(__cdecl*)();

		auto mono_get_root_export = process_.modules().GetExport(module_data_ptr_, "mono_get_root_domain");

		blackbone::RemoteFunction<mono_get_root_domain> mono_get_root_function{process_, mono_get_root_export->procAddress};
		decltype(mono_get_root_function)::CallArguments args{};

		auto result = mono_get_root_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_get_root_domain");

		return result.result();
	}

	intptr_t Injector::GetDomain()
	{
		using mono_domain_get = intptr_t(__cdecl*)();

		auto mono_domain_get_export = process_.modules().GetExport(module_data_ptr_, "mono_domain_get");

		blackbone::RemoteFunction<mono_domain_get> mono_domain_get_function{process_, mono_domain_get_export->procAddress};
		decltype(mono_domain_get_function)::CallArguments args{};

		auto result = mono_domain_get_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_domain_get");

		return result.result();
	}

	intptr_t Injector::ThreadAttach(intptr_t domain)
	{
		using mono_thread_attach = intptr_t(__cdecl*)(intptr_t /*domain*/);

		auto mono_thread_attach_export = process_.modules().GetExport(module_data_ptr_, "mono_thread_attach");

		blackbone::RemoteFunction<mono_thread_attach> mono_thread_attach_function{process_, mono_thread_attach_export->procAddress};
		decltype(mono_thread_attach_function)::CallArguments args{
			domain
		};

		auto result = mono_thread_attach_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_thread_attach");

		return result.result();
	}

	void Injector::SecuritySetMode(int mode)
	{
		using mono_security_set_mode = void(__cdecl*)(int /*mode*/);

		auto mono_security_set_mode_export = process_.modules().GetExport(module_data_ptr_, "mono_security_set_mode");

		blackbone::RemoteFunction<mono_security_set_mode> mono_security_set_mode_function{process_, mono_security_set_mode_export->procAddress};
		decltype(mono_security_set_mode_function)::CallArguments args{
			mode
		};

		auto result = mono_security_set_mode_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_security_set_mode");
	}

	intptr_t Injector::ImageOpenFromDataFull(const std::vector<char>& image)
	{
		using mono_image_open_from_data = intptr_t(__cdecl*)(intptr_t /*image*/, uint32_t /*data_len*/, int /*need_copy*/, int* /*status*/, int /*refonly*/);

		auto mono_image_open_from_data_export = process_.modules().GetExport(module_data_ptr_, "mono_image_open_from_data_full");

		auto memblock = process_.memory().Allocate(image.size(), PAGE_READWRITE);
		memblock->Write(0, image.size(), image.data());

		int status;

		blackbone::RemoteFunction<mono_image_open_from_data> mono_image_open_from_data_function{process_, mono_image_open_from_data_export->procAddress};
		decltype(mono_image_open_from_data_function)::CallArguments args{
			memblock->ptr<intptr_t>(),
			static_cast<uint32_t>(image.size()),
			1,
			&status,
			0
		};

		auto result = mono_image_open_from_data_function.Call(args, context_thread_);

		memblock->Free();

		if (!result.success())
			throw std::runtime_error("Could not call mono_image_open_from_data_full");

		return result.result();
	}

	intptr_t Injector::AssemblyLoadFromFull(intptr_t raw_image)
	{
		using mono_assembly_load_from = intptr_t(__cdecl*)(intptr_t /*image*/, int* /*fname*/, int* /*status*/, bool /*refonly*/);

		auto mono_assembly_load_from_export = process_.modules().GetExport(module_data_ptr_, "mono_assembly_load_from_full");

		int status;

		blackbone::RemoteFunction<mono_assembly_load_from> mono_assembly_load_from_function{process_, mono_assembly_load_from_export->procAddress};
		decltype(mono_assembly_load_from_function)::CallArguments args{
			raw_image,
			nullptr,
			&status,
			false
		};

		auto result = mono_assembly_load_from_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_assembly_load_from_full");

		return result.result();
	}

	intptr_t Injector::AssemblyGetImage(intptr_t assembly)
	{
		using mono_assembly_get_image = intptr_t(__cdecl*)(intptr_t /*assembly*/);

		auto mono_assembly_get_image_export = process_.modules().GetExport(module_data_ptr_, "mono_assembly_get_image");

		blackbone::RemoteFunction<mono_assembly_get_image> mono_assembly_get_image_function{process_, mono_assembly_get_image_export->procAddress};
		decltype(mono_assembly_get_image_function)::CallArguments args{
			assembly
		};

		auto result = mono_assembly_get_image_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_assembly_get_image");

		return result.result();
	}

	intptr_t Injector::GetClassFromName(intptr_t image, const char* name_space, const char* class_name)
	{
		using mono_class_from_name = intptr_t(__cdecl*)(intptr_t /*image*/, const char* /*name_space*/, const char* /*class*/);

		auto mono_class_from_name_export = process_.modules().GetExport(module_data_ptr_, "mono_class_from_name");

		blackbone::RemoteFunction<mono_class_from_name> mono_class_from_name_function{process_, mono_class_from_name_export->procAddress};
		decltype(mono_class_from_name_function)::CallArguments args{
			image,
			name_space,
			class_name
		};

		auto result = mono_class_from_name_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_class_from_name");

		return result.result();
	}

	intptr_t Injector::GetMethodFromName(intptr_t class_id, const char* method_name)
	{
		using mono_class_get_method_from_name = intptr_t(__cdecl*)(intptr_t /*class*/, const char* /*method_name*/, int /*param_count*/);

		auto mono_class_get_method_export = process_.modules().GetExport(module_data_ptr_, "mono_class_get_method_from_name");

		blackbone::RemoteFunction<mono_class_get_method_from_name> mono_class_get_method_function{process_, mono_class_get_method_export->procAddress};
		decltype(mono_class_get_method_function)::CallArguments args{
			class_id,
			method_name,
			0
		};

		auto result = mono_class_get_method_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_class_get_method_from_name");

		return result.result();
	}

	intptr_t Injector::RuntimeInvokeMethod(intptr_t method_id)
	{
		using mono_runtime_invoke = intptr_t(__cdecl*)(intptr_t /*method*/, void* /*obj*/, void** /*params*/, int** /*exc*/);

		auto mono_runtime_invoke_export = process_.modules().GetExport(module_data_ptr_, "mono_runtime_invoke");

		blackbone::RemoteFunction<mono_runtime_invoke> mono_runtime_invoke_function{process_, mono_runtime_invoke_export->procAddress};
		decltype(mono_runtime_invoke_function)::CallArguments args{
			method_id,
			nullptr,
			nullptr,
			nullptr
		};

		auto result = mono_runtime_invoke_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_runtime_invoke");

		return result.result();
	}

	int Injector::ThreadDetach(intptr_t domain)
	{
		using mono_thread_detach = int(__cdecl*)(intptr_t /*domain*/);

		auto mono_thread_detach_export = process_.modules().GetExport(module_data_ptr_, "mono_thread_detach");

		blackbone::RemoteFunction<mono_thread_detach> mono_thread_detach_function{process_, mono_thread_detach_export->procAddress};
		decltype(mono_thread_detach_function)::CallArguments args{
			domain
		};

		auto result = mono_thread_detach_function.Call(args, context_thread_);
		if (!result.success())
			throw std::runtime_error("Could not call mono_thread_detach");

		return result.result();
	}

	NTSTATUS Injector::UnlinkAssembly(intptr_t domain)
	{
		// Last loaded assembly should be the first in the array

		auto pAsm = blackbone::AsmFactory::GetAssembler(process_.modules().GetMainModule()->type);
		auto& a = *pAsm;

		a.GenPrologue();

		a->push(a->zdi);
		a->push(a->zcx);

		a->mov(a->zdi, domain);

#if _WIN64
		a->add(a->zdi, 0xc0);
#else
		a->add(a->zdi, 0x6c);
#endif

		a->mov(a->zcx, a->intptr_ptr(a->zdi)); // GSList* tmp = domain->domain_assemblies

#if _WIN64
		a->add(a->zcx, 0x8);
#else
		a->add(a->zcx, 0x4);
#endif

		a->mov(a->zcx, a->intptr_ptr(a->zcx)); // tmp = tmp->next
		a->mov(a->intptr_ptr(a->zdi), a->zcx); // domain->domain_assemblies = tmp

		a->pop(a->zcx);
		a->pop(a->zdi);

		process_.remote().AddReturnWithEvent(a);

		a.GenEpilogue();

		uint64_t result = NULL;
		auto status = process_.remote().ExecInWorkerThread(a->make(), a->getCodeSize(), result);

		return status;
	}
}
