
// Handle Cemu function hooking

#include <ws2tcpip.h>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <cstring>
#include <detours.h>
#include "cemuhook_inject.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "detours.lib")

/*---------------------------------------
|      Find unexported functions        |
---------------------------------------*/

// Search program memory for unexported function by signature
// --------------------------------------------------------------------------------------
DWORD_PTR FindUnexportedFunction(PBYTE signature, DWORD length)
{
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
	for (size_t i = 0; i < size; i++)
	{
		bool match = true;
		for (size_t j = 0; j < length; j++)
		{
			// Read one byte and compare it to the signature
			unsigned char memory_value;
			std::memcpy(&memory_value, base + i + j, 1);

			// If any part of the memory does not match the sequence, fail
			if (memory_value != signature[j])
			{
				match = false;
			}
		}

		if (match)
		{
			// Return real address
			return (DWORD_PTR)(base + i);
		}
	}
	// -------------------------------------------

	// Return 0 if not found
	return 0;
}

/*---------------------------------
|      Replace URLS               |
----------------------------------*/

// -------------------------------------------
void InstallHook()
{
	PBYTE address = (PBYTE)"https://account.nintendo.net";

	size_t length = 29 - 1;  //- 1 b/c of null
	DWORD_PTR address_ptr = FindUnexportedFunction(address, length);

	char buffer[29] = "http://c.account.pretendo.cc";

	DWORD old;
	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);

	//do it again b/c there's 2 account URLs in Cemu

	address_ptr = FindUnexportedFunction(address, length); //- 1 b/c of null

	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);
	// -------------------------------------------

	// Check if "cemuhook.dll" exists
	// -------------------------------------------
	if (cemuhook_exists())
	{
		// Inject DLL if found
		inject_cemuhook();
	}
	// -------------------------------------------

	//MessageBox(NULL, "Ready!", "Pretendo Cemu Patch", MB_OK);
}
// -------------------------------------------

/*---------------------------------
|        Revert URL changes       |
----------------------------------*/

// -------------------------------------------
void RemoveHook()
{
	PBYTE address = (PBYTE)"http://c.account.pretendo.cc";

	const size_t length = 29 - 1;
	DWORD_PTR address_ptr = FindUnexportedFunction(address,length);

	char buffer[29] = "https://account.nintendo.net";

	DWORD old;
	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);

	//do it again b/c there's 2 account URLs in Cemu
	address_ptr = FindUnexportedFunction(address, length);

	VirtualProtect((LPVOID)address_ptr, length, PAGE_EXECUTE_READWRITE, &old);
	memcpy((LPVOID)address_ptr, buffer, length);
	VirtualProtect((LPVOID)address_ptr, length, old, nullptr);
}
// -------------------------------------------