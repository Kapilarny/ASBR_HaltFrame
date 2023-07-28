#pragma once

#include <string>
#include <vector>
#include <Windows.h>

#define STORM_EXPORT __declspec(dllexport)

namespace plugin
{
	inline __int64 moduleBase;
	inline HWND windowHandle;
	inline DWORD processId;
	inline HANDLE processHandle;
};

extern "C"
{
	STORM_EXPORT void __stdcall InitializePlugin(__int64 a, std::vector<__int64> b);
	STORM_EXPORT void __stdcall InitializeCommands(__int64 a, __int64 addCommandFunctionAddress);
	STORM_EXPORT void __stdcall InitializeHooks(__int64 a, __int64 hookFunctionAddress);
	STORM_EXPORT void __stdcall InitializeLuaCommands(__int64 a, __int64 addCommandFunction);
	STORM_EXPORT void __stdcall GameLoop(__int64 a);
	STORM_EXPORT bool __stdcall ParseApiFiles(__int64 a, std::string filePath, std::vector<char> fileBytes);
}