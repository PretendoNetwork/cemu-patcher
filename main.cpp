// DLL entry point

#include <windows.h>
#include "hook.h"

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		InstallHook();
		break;

	case DLL_PROCESS_DETACH:
		RemoveHook();
		break;
	}

	return TRUE;
}