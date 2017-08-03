# MonoInjector
Mono Assembly Injector for Unity3D games

## Features
- Supports x86 and x64 processes
- Loads assemblies from memory
- Unlink assembly feature. Hides injected assembly from `GetAssemblies()`. Experimental. Add `--unlink` to enable it.

## Usage
Running MonoInjector.exe with the `--help` argument will display all possible options.

`MonoInjector.exe --help`

#### Example:
`MonoInjector.exe --target target.exe --dll TestDll.dll --namespace TestDll --class Load --method Init`

Or

`MonoInjector.exe -t target.exe -d TestDll.dll -n TestDll -c Load -m Init`
