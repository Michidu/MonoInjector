#include <iostream>

#include "tools.h"
#include "Injector.h"

#include "args/args.hxx"

int main(int argc, char* argv[])
{
	args::ArgumentParser parser{"Mono Assembly Injector V1.0", "https://github.com/Michidu/MonoInjector"};
	args::HelpFlag help{parser, "help", "Display help menu", {'h', "help"}};
	args::Group group{parser, "Required arguments:", args::Group::Validators::All};
	args::ValueFlag<std::string> process_name_value{group, "target", "Target process name", {'t', "target"}};
	args::ValueFlag<std::string> dll_name_value{group, "dll", "DLL file name", {'d', "dll"}};
	args::ValueFlag<std::string> name_space_value{group, "namespace", "Namespace of the Init class", {'n', "namespace"}};
	args::ValueFlag<std::string> class_name_value{group, "class", "Init class", {'c', "class"}};
	args::ValueFlag<std::string> method_name_value{group, "method", "Init method", {'m', "method"}};
	args::Flag unlink(parser, "unlink", "Unlink assembly (Experimental)", {"unlink"});

	try
	{
		parser.ParseCLI(argc, argv);
	}
	catch (const args::Help&)
	{
		std::cout << parser;
		return 0;
	}
	catch (const args::ParseError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return -1;
	}
	catch (const args::ValidationError& e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return -1;
	}

	std::wcout << "Mono Assembly Injector V1.0" << std::endl << std::endl;

	std::wstring process_name = blackbone::Utils::AnsiToWstring(args::get(process_name_value));
	std::string dll_name = args::get(dll_name_value);
	std::string name_space = args::get(name_space_value);
	std::string class_name = args::get(class_name_value);
	std::string method_name = args::get(method_name_value);

	if (!MonoInjector::tools::IsFileExists(dll_name))
	{
		std::cout << "File " << dll_name.c_str() << " does not exist" << std::endl;
		return -1;
	}

	DWORD pid = MonoInjector::tools::GetProcessPid(process_name);
	if (pid == 0)
	{
		std::wcout << "Could not find " << process_name.c_str() << std::endl;
		return -1;
	}

	MonoInjector::Injector injector;
	MonoInjector::AssemblyInfo assembly_info{dll_name, name_space, class_name, method_name, unlink};

	bool result = injector.InjectDll(pid, assembly_info);
	if (!result)
	{
		std::wcout << "Failed to inject dll" << std::endl;
		return -1;
	}

	std::wcout << std::endl << "Successfully injected dll!" << std::endl << std::endl;

	system("pause");

	return 0;
}
