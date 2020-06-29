#include "pch.h"
#include <Psapi.h>
#include <mhook-lib/mhook.h>

#define DEFHOOKFUNCTYPE(func) \
	typedef decltype (func) fn##func; \
	auto g_pfn##func = func;

DEFHOOKFUNCTYPE(GetModuleFileNameExA);

#undef DEFHOOKFUNCTYPE

DWORD WINAPI MyGetModuleFileNameExA( HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
	if (hModule == 0)
	{
		auto n = nSize;
		if (!QueryFullProcessImageNameA(hProcess, 0, lpFilename, &n))
		{
			n = 0;
		}
		return n;
	}
	return g_pfnGetModuleFileNameExA(hProcess, hModule, lpFilename, nSize);
}

void Init()
{
	Mhook_SetHook((PVOID*)&g_pfnGetModuleFileNameExA, (PVOID)MyGetModuleFileNameExA);
}

void Unload()
{
	Mhook_Unhook((PVOID*)&g_pfnGetModuleFileNameExA);
}
