#include "Main.h"

#include <d3d11.h>
#include <Xinput.h>

#include <stdio.h>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

#include "Thirdparty/MinHook.h"

static const unsigned int NumberOfKeys = 256U;
bool previousKeyboardState[NumberOfKeys];
static bool isHalted = false;

// Create a hook for XInputGetKeystroke.
// This hook will be used to halt the game loop.
typedef void(__stdcall* SteamAPI_RunCallbacks)();
static SteamAPI_RunCallbacks SteamAPI_RunCallbacks_Original = nullptr;

bool isKeyDown(int key)
{
	return (GetAsyncKeyState(key) & (1 << 16));
}

//Misses key presses when application is bogged down.
bool isKeyFirstPressed(int key)
{
	bool previousState = previousKeyboardState[key];

	previousKeyboardState[key] = isKeyDown(key);

	return (previousKeyboardState[key] && !previousState);
}

//Misses key releases when application is bogged down.
bool isKeyFirstReleased(int key)
{
	bool previousState = previousKeyboardState[key];

	previousKeyboardState[key] = isKeyDown(key);

	return (!previousKeyboardState[key] && previousState);
}

void __stdcall SteamAPI_RunCallbacks_Hook()
{
	if (isKeyFirstPressed(VK_F2)) {
		isHalted = !isHalted;
	}

	if (isHalted) {
		Sleep(20);

		// Use WindowsAPI to call Window Callbacks, otherwise the game will freeze.
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		SteamAPI_RunCallbacks_Hook();
	}

	SteamAPI_RunCallbacks_Original();
}

void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess)
{
	DWORD oldprotect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	if (!WriteProcessMemory(hProcess, dst, src, size, nullptr)) {
		printf("[HaltFrame]: Failed to write to memory!\n");

		// Print the error code.
		printf("[HaltFrame]: Error code: %d\n", GetLastError());

		return;
	}
	VirtualProtectEx(hProcess, dst, size, oldprotect, &oldprotect);
}

void WaitAndGetWindow() {
	plugin::windowHandle = FindWindowA(NULL, "Jojo's Bizarre Adventure: All-Star Battle R");
	if (!plugin::windowHandle) { // If the window handle is null, it means that the game didnt initialize the window yet. Wait 10ms and try again until it we get the handle.
		Sleep(10); 
		WaitAndGetWindow();
	}
}

void WaitForSteamAPIToLoad() {
	if (!GetModuleHandleA("steam_api64.dll")) { // If steam_api64.dll is not loaded, wait 10ms and try again until it is loaded.
		Sleep(10);
		WaitForSteamAPIToLoad();
	}
}

// This function is called when booting the game.
// In the modding api, 0xC00 is added to the module base by default. In my modified code, I am removing it.
void InitializePlugin(__int64 a, std::vector<__int64> b)
{
	plugin::moduleBase = a - 0xC00;

	// Get the window handle and process id.
	WaitAndGetWindow();

	GetWindowThreadProcessId(plugin::windowHandle, &plugin::processId);

	if (!plugin::processId) {
		printf("[HaltFrame]: Failed to get process id!\n");
		return;
	}

	// Get the process handle.
	plugin::processHandle = OpenProcess(PROCESS_ALL_ACCESS, true, plugin::processId);

	if (!plugin::processHandle) {
		printf("[HaltFrame]: Failed to get process handle!\n");
		return;
	}

	if (MH_Initialize() != MH_OK) {
		printf("[HaltFrame]: Failed to initialize MinHook!\n");
		return;
	}

	//Get the current state of each key as the application starts to ensure that keys held down beforehand are not processed as pressed keys.
	for (unsigned int keyNum = 0U; keyNum < NumberOfKeys; ++keyNum)
	{
		previousKeyboardState[keyNum] = isKeyDown(keyNum);
	}

	WaitForSteamAPIToLoad();

	// Create a hook for XInputGetState.
	LPVOID hookAddr = GetProcAddress(GetModuleHandleA("steam_api64.dll"), "SteamAPI_RunCallbacks");
	bool status = MH_CreateHook(hookAddr, SteamAPI_RunCallbacks_Hook, (LPVOID*)&SteamAPI_RunCallbacks_Original);
	if (status != MH_OK) {
		printf("[HaltFrame]: Failed to create hook!\n");
		return;
	}

	status = MH_EnableHook(hookAddr);
	if (status != MH_OK) {
		printf("[HaltFrame]: Failed to enable hook!\n");
		return;
	}

	printf("[HaltFrame]: Initialized!\n");
}

// This function adds commands to the API's console.
void InitializeCommands(__int64 a, __int64 addCommandFunctionAddress) {}

// Use this function to hook any of the game's original functions.
void InitializeHooks(__int64 a, __int64 hookFunctionAddress) {}

// Use this function to add any lua commands to the game.
void InitializeLuaCommands(__int64 a, __int64 addCommandFunction) {}

// This function will be called all the time while you're playing after the plugin has been initialized.
void GameLoop(__int64 a) {
	// This function does not work in the current version of storm lmfao
}

// This function is called when the API is loading a mod's files. Return true if the file was read by this plugin, otherwise return false for the API to manage the file.
bool ParseApiFiles(__int64 a, std::string filePath, std::vector<char> fileBytes)
{
	return false;
}
