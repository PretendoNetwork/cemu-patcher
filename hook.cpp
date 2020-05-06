// Handle the hooking

#include <ws2tcpip.h>
#include <string>
#include <windows.h>
#include <detours.h>
#include "cemuhook_inject.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "detours.lib")

int (WSAAPI* getaddrinfo_original)(
	PCSTR            pNodeName,
	PCSTR            pServiceName,
	const ADDRINFOA* pHints,
	PADDRINFOA*      ppResult
) = getaddrinfo;

int WSAAPI getaddrinfo_hook(
	PCSTR            pNodeName,
	PCSTR            pServiceName,
	const ADDRINFOA* pHints,
	PADDRINFOA* ppResult
)
{
	// Dirty check for Nintendo requests
	std::string nintendDomain = "nintendo.net";
	int nintendoCheck = std::string(pNodeName).find(nintendDomain);
	if (nintendoCheck != std::string::npos)
	{
		std::string text = "Redirecting Nintendo request: ";
		text = text + pServiceName + "://" + pNodeName;

		MessageBox(NULL, text.c_str(), "getaddrinfo", MB_OK);

		std::string pNewNodeName = std::string(pNodeName).replace(nintendoCheck, nintendDomain.length(), "pretendo.cc");

		return getaddrinfo_original(pNewNodeName.c_str(), pServiceName, pHints, ppResult);
	}

	return getaddrinfo_original(pNodeName, pServiceName, pHints, ppResult);
}

void InstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)getaddrinfo_original, getaddrinfo_hook);
	DetourTransactionCommit();

	if (cemuhook_exists())
	{
		inject_cemuhook();
	}

	MessageBox(NULL, "Installed hook", "Pretendo Cemu Patch", MB_OK);
}

void RemoveHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)getaddrinfo_original, getaddrinfo_hook);
	DetourTransactionCommit();

	MessageBox(NULL, "Removed hook", "Pretendo Cemu Patch", MB_OK);
}