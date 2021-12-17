
// Handle Cemu function hooking

#include <ws2tcpip.h>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <cstring>
#include <detours.h>
#include "cemuhook_inject.h"
#include <iostream>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "detours.lib")

/*---------------------------------------
|      Find unexported addresses        |
---------------------------------------*/

// Search program memory for unexported function by signature
// --------------------------------------------------------------------------------------
DWORD_PTR FindUnexportedAddress(PBYTE signature, DWORD length) {
	// Get mod info
	// -------------------------------------------
	HMODULE hMod = GetModuleHandleA(NULL);
	MODULEINFO modInfo = { NULL };

	GetModuleInformation(GetCurrentProcess(), hMod, &modInfo, sizeof(modInfo));
	// -------------------------------------------

	// Search params
	// -------------------------------------------
	DWORD size = modInfo.SizeOfImage;
	char* base = (char*)hMod;
	// -------------------------------------------

	// Loop over memory checking for signature
	// -------------------------------------------
	for (size_t i = 0; i < size; i++) {
		bool match = true;

		for (size_t j = 0; j < length; j++) {
			// Read one byte and compare it to the signature
			unsigned char memory_value;
			std::memcpy(&memory_value, base + i + j, 1);

			// If any part of the memory does not match the sequence, fail
			if (memory_value != signature[j]) {
				match = false;
				break;
			}
		}

		if (match) {
			// Return real address
			return (DWORD_PTR)(base + i);
		}
	}
	// -------------------------------------------

	// Return 0 if not found
	return 0;
}
// -------------------------------------------

/*-------------------------------
|      Window title hooking      |
--------------------------------*/

HWND main_window = NULL;

HWND(__stdcall* CreateWindowExW_original)(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam
) = CreateWindowExW;

HWND CreateWindowExW_hook(
	DWORD     dwExStyle,
	LPCWSTR   lpClassName,
	LPCWSTR   lpWindowName,
	DWORD     dwStyle,
	int       X,
	int       Y,
	int       nWidth,
	int       nHeight,
	HWND      hWndParent,
	HMENU     hMenu,
	HINSTANCE hInstance,
	LPVOID    lpParam
) {
	// Dirty check to get the main window pointer
	// Please change
	if (main_window == NULL && lpWindowName != NULL && hWndParent == NULL) {
		std::wstring original_title(lpWindowName);
		std::wstring prefix = L"[Pretendo] ";
		std::wstring new_title = prefix + original_title;

		lpWindowName = new_title.c_str();
	}

	HWND window = CreateWindowExW_original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	
	if (main_window == NULL && lpWindowName != NULL && hWndParent == NULL) {
		main_window = window;
	}
	
	return window;
}

BOOL(__stdcall* SetWindowTextW_original)(HWND hWnd, LPCWSTR lpString) = SetWindowTextW;

BOOL SetWindowTextW_hook(HWND hWnd, LPCWSTR lpString) {
	if (hWnd == main_window) {
		std::wstring original_title(lpString);
		std::wstring prefix = L"[Pretendo] ";
		std::wstring new_title = prefix + original_title;

		lpString = new_title.c_str();
	}

	return SetWindowTextW_original(hWnd, lpString);
}

/*---------------------------------
|      Replace URLS               |
----------------------------------*/

// -------------------------------------------
void InstallHook() {
	PBYTE address = (PBYTE)"https://account.nintendo.net";

	size_t length = 29 - 1;  //- 1 b/c of null
	DWORD_PTR address_ptr = FindUnexportedAddress(address, length);

	char buffer[29] = "http://c.account.pretendo.cc";

	DWORD old;
	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);

	//do it again b/c there's 2 account URLs in Cemu

	address_ptr = FindUnexportedAddress(address, length); //- 1 b/c of null

	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);
	// -------------------------------------------

	// Setup Detours
	// -------------------------------------------
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)CreateWindowExW_original, CreateWindowExW_hook);
	DetourAttach(&(PVOID&)SetWindowTextW_original, SetWindowTextW_hook);
	DetourTransactionCommit();

	// Check if "true_cemuhook.dll" exists
	// -------------------------------------------
	if (cemuhook_exists()) {
		// Inject DLL if found
		inject_cemuhook();
	}
	// -------------------------------------------

	DWORD current_process_id = GetCurrentProcessId();
	HANDLE cemu_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, current_process_id);

	SetWindowTextA(GetActiveWindow(), "HI");

	//MessageBox(NULL, "Ready!", "Pretendo Cemu Patch", MB_OK);
}
// -------------------------------------------

/*---------------------------------
|        Revert URL changes       |
----------------------------------*/

// -------------------------------------------
void RemoveHook() {
	PBYTE address = (PBYTE)"http://c.account.pretendo.cc";

	const size_t length = 29 - 1;
	DWORD_PTR address_ptr = FindUnexportedAddress(address,length);

	char buffer[29] = "https://account.nintendo.net";

	DWORD old;
	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);

	//do it again b/c there's 2 account URLs in Cemu
	address_ptr = FindUnexportedAddress(address, length);

	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)CreateWindowExW_original, CreateWindowExW_hook);
	DetourDetach(&(PVOID&)SetWindowTextW_original, SetWindowTextW_hook);
	DetourTransactionCommit();
}
// -------------------------------------------