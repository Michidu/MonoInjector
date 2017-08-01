# MonoInjector
Mono Assembly Injector for Unity3D games

## Features
- Supports x86 and x64 processes
- Loads assemblies from memory
- Unlink assembly feature (hides it from GetAssemblies(), experimental)

## Usage
`MonoInjector.exe -t target.exe -d TestDll.dll -n TestDll -c Load -m Init --unlink`
