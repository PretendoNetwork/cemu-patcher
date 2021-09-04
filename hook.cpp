
// Handle Cemu function hooking

#include <ws2tcpip.h>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <cstring>
#include <detours.h>
#include <curl/curl.h>
#include "cemuhook_inject.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "detours.lib")

/*---------------------------------------
|      Find unexported functions        |
---------------------------------------*/

// Search program memory for unexported function by signature
// --------------------------------------------------------------------------------------
DWORD_PTR FindUnexportedFunction(PBYTE signature)
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
		for (size_t j = 0; j < sizeof(signature); j++)
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
// --------------------------------------------------------------------------------------

/*---------------------------------------
|           nlibcurl hooking            |
---------------------------------------*/

DWORD_PTR address_nlibcurl_easy_init; // Address for nlibcurl.curl_easy_init in Cemu
DWORD_PTR address_nlibcurl_vsetopt;   // Address for nlibcurl.Curl_vsetopt in Cemu


static CURLcode hook_Curl_vsetopt(CURL* curl, CURLoption option, va_list arg);
static CURLcode hook_curl_easy_getinfo(CURL *curl, CURLINFO info, void* p);

decltype(&hook_Curl_vsetopt) original_Curl_vsetopt;
decltype(&curl_easy_init) original_curl_easy_init;
decltype(&curl_easy_cleanup) original_curl_easy_cleanup;
decltype(&hook_curl_easy_getinfo) original_curl_easy_getinfo;

bool proxyEnabled = true;

struct CurlHookData
{
	char* pvt = nullptr;
};

static CurlHookData* getHookData(CURL* curl)
{
	CurlHookData* pvt = nullptr;
	original_curl_easy_getinfo(curl, CURLINFO_PRIVATE, &pvt);
	return pvt;
}

static CURLcode original_curl_easy_setopt(CURL* curl, CURLoption option, ...)
{
	va_list arg;
	CURLcode result;

	if (!curl)
		return CURLE_BAD_FUNCTION_ARGUMENT;

	va_start(arg, option);

	result = original_Curl_vsetopt(curl, option, arg);

	va_end(arg);
	return result;
}

CURLcode hook_Curl_vsetopt(CURL* curl, CURLoption option, va_list param)
{
	MessageBox(NULL, "hook_Curl_vsetopt", "hook_Curl_vsetopt", MB_OK);

	va_list arg;
	va_copy(arg, param);
	switch (option)
	{
	case CURLOPT_URL:
	{
		auto url = va_arg(param, char*);
		//dlogp("curl: 0x%p, CURLOPT_URL: '%s'", curl, url);
		break;
	}
	case CURLOPT_HTTPHEADER:
	{
		auto headers = va_arg(param, struct curl_slist*);
		//auto str = slist2str(headers);
		//dlogp("curl: 0x%p, CURLOPT_HTTPHEADER: %s", curl, str.c_str());
		break;
	}
	case CURLOPT_POSTFIELDS:
	{
		auto postdata = va_arg(param, char*);
		if (!postdata)
		{
			//postdata = "<nullptr>";
		}
			
		break;
	}
	case CURLOPT_POSTFIELDSIZE:
	{
		auto size = va_arg(param, long);
		break;
	}
	case CURLOPT_PRIVATE:
	{
		auto pvt = va_arg(param, void*);
		auto result = original_curl_easy_getinfo(curl, CURLINFO_PRIVATE, pvt);
		if (result == CURLE_OK)
		{
			auto storedpvt = *(CurlHookData**)pvt;
			*(char**)pvt = storedpvt->pvt;
		}
		va_end(param);
		return result;
	}
	case CURLOPT_SSL_VERIFYPEER:
	{
		auto verify = va_arg(param, long);
		if (proxyEnabled)
		{
			auto result = original_curl_easy_setopt(curl, option, 0);
			va_end(param);
			return result;
		}
		break;
	}
	case CURLOPT_SSL_VERIFYHOST:
	{
		auto verify = va_arg(param, long);
		if (proxyEnabled)
		{
			auto result = original_curl_easy_setopt(curl, option, 0);
			va_end(param);
			return result;
		}
		break;
	}
	case CURLOPT_WRITEFUNCTION:
	{
		auto writefunc = va_arg(param, curl_write_callback);
		break;
	}
	case CURLOPT_WRITEDATA:
	{
		auto data = va_arg(param, void*);
		break;
	}
	default:
	{
		break;
	}
	}
	va_end(param);
	return original_Curl_vsetopt(curl, option, arg);
}

// --------------------------------------------------------------------------------------
CURL* nlibcurl_easy_init_hook()
{
	MessageBox(NULL, "Hooked nlibcurl.curl_easy_init", "Debug", MB_OK);

	CURL* curl = curl_easy_init();
		
	if (curl)
	{
		// hook curl functions to disable SSL verification
		/*
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_PROXY_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_PROXY_SSL_VERIFYHOST, 0);
		curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_SSL_CTX_DATA, NULL);
		curl_easy_setopt(curl, CURLOPT_CERTINFO, 0);
		*/

		// Try to set a proxy server?
		curl_easy_setopt(curl, CURLOPT_PROXY, "http://192.168.0.12:8080");
	}
	else
	{
		MessageBox(NULL, "Failed to call curl_easy_init", "Debug", MB_OK);
	}

	return curl;
}
// --------------------------------------------------------------------------------------

/*---------------------------------
|      getaddrinfo hooking        |
----------------------------------*/

// Get the original getaddrinfo function
// -------------------------------------------
int (WSAAPI* getaddrinfo_original)(
	PCSTR            pNodeName,
	PCSTR            pServiceName,
	const ADDRINFOA* pHints,
	PADDRINFOA* ppResult
	) = getaddrinfo;
// -------------------------------------------


// Create hook function for getaddrinfo
// -------------------------------------------
int WSAAPI getaddrinfo_hook(
	PCSTR            pNodeName,
	PCSTR            pServiceName,
	const ADDRINFOA* pHints,
	PADDRINFOA* ppResult
)
{
	// Dirty check for Nintendo requests
	std::string nintendoDomain = "nintendo.net";
	size_t nintendoCheck = std::string(pNodeName).find(nintendoDomain);

	if (nintendoCheck != std::string::npos)
	{

		// Replace "nintendo.net" in domain with "pretendo.cc"
		std::string pNewNodeName = std::string(pNodeName).replace(nintendoCheck, nintendoDomain.length(), "pretendo.cc");

		pNodeName = pNewNodeName.c_str();

		MessageBox(NULL, pNodeName, "Debug", MB_OK);

		//return getaddrinfo_original(pNodeName, pServiceName, pHints, ppResult);
	}

	return getaddrinfo_original(pNodeName, pServiceName, pHints, ppResult);
}
// -------------------------------------------

/*---------------------------------
|      Attach Detours hooks       |
----------------------------------*/

// -------------------------------------------
void InstallHook()
{
	// Get cURL function addresses
	// -------------------------------------------
	PBYTE signature_nlibcurl_easy_init = (PBYTE)"\x48\x89\x4C\x24\x08\x55\x53\x57\x48\x8D\x6C\x24\xB9\x48\x81\xEC";
	PBYTE signature_nlibcurl_vsetopt = (PBYTE)"\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x57\x48\x83\xEC\x28\x8B";

	address_nlibcurl_easy_init = FindUnexportedFunction(signature_nlibcurl_easy_init);
	address_nlibcurl_vsetopt   = FindUnexportedFunction(signature_nlibcurl_vsetopt);
	// -------------------------------------------

	// Setup Detours
	// -------------------------------------------
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)getaddrinfo_original, getaddrinfo_hook);
	DetourAttach(&(PVOID&)address_nlibcurl_easy_init, nlibcurl_easy_init_hook);
	//DetourAttach(&(PVOID&)address_nlibcurl_vsetopt,   hook_Curl_vsetopt);
	DetourTransactionCommit();
	// -------------------------------------------

	// Check if "cemuhook.dll" exists
	// -------------------------------------------
	if (cemuhook_exists())
	{
		// Inject DLL if found
		inject_cemuhook();
	}
	// -------------------------------------------

	curl_global_init(CURL_GLOBAL_DEFAULT); // Things be crashing without this bad boy

	MessageBox(NULL, "Installed hook", "Pretendo Cemu Patch", MB_OK);
}
// -------------------------------------------

/*---------------------------------
|     Detach Detours hooks       |
----------------------------------*/

// -------------------------------------------
void RemoveHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)getaddrinfo_original, getaddrinfo_hook);
	DetourDetach(&(PVOID&)address_nlibcurl_easy_init, nlibcurl_easy_init_hook);
	DetourTransactionCommit();
}
// -------------------------------------------