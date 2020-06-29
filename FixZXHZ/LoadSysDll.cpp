#include "pch.h"
#include <intrin.h>

#define EXTERNC extern "C"

#define FUNCTION EXTERNC int __cdecl

#define NOP_FUNC { \
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	__nop();\
	return __COUNTER__;\
}
// 用 __COUNTER__ 来生成一点不一样的代码，避免被 VS 自动合并相同函数

#ifdef _WIN64
#define PREFIX "_"
#else
#define PREFIX "__"
#endif

#define PRAGMA(api) comment(linker, "/EXPORT:" #api "=" PREFIX #api)
#define EXPORT(api) FUNCTION _##api() NOP_FUNC


#pragma region 声明导出函数
// 声明导出函数
#pragma PRAGMA(AlphaBlend)
#pragma PRAGMA(GradientFill)
#pragma PRAGMA(DllInitialize)
#pragma PRAGMA(TransparentBlt)
#pragma PRAGMA(vSetDdrawflag)

EXPORT(AlphaBlend)
EXPORT(GradientFill)
EXPORT(DllInitialize)
EXPORT(TransparentBlt)
EXPORT(vSetDdrawflag)
#pragma endregion

#pragma region 还原导出函数
bool WriteMemory(PBYTE BaseAddress, PBYTE Buffer, DWORD nSize)
{
	DWORD ProtectFlag = 0;
	if (VirtualProtectEx(GetCurrentProcess(), BaseAddress, nSize, PAGE_EXECUTE_READWRITE, &ProtectFlag))
	{
		memcpy(BaseAddress, Buffer, nSize);
		FlushInstructionCache(GetCurrentProcess(), BaseAddress, nSize);
		VirtualProtectEx(GetCurrentProcess(), BaseAddress, nSize, ProtectFlag, &ProtectFlag);
		return true;
	}
	return false;
}

// 定义MWORD为机器字长
#include <stdint.h>
#ifdef _WIN64
typedef uint64_t MWORD;
#else
typedef uint32_t MWORD;
#endif

// 还原导出函数
void InstallJMP(PBYTE BaseAddress, MWORD Function)
{
#ifdef _WIN64
	BYTE move[] = { 0x48, 0xB8 };//move rax,xxL);
	BYTE jump[] = { 0xFF, 0xE0 };//jmp rax

	WriteMemory(BaseAddress, move, sizeof(move));
	BaseAddress += sizeof(move);

	WriteMemory(BaseAddress, (PBYTE)&Function, sizeof(MWORD));
	BaseAddress += sizeof(MWORD);

	WriteMemory(BaseAddress, jump, sizeof(jump));
#else
	BYTE jump[] = { 0xE9 };
	WriteMemory(BaseAddress, jump, sizeof(jump));
	BaseAddress += sizeof(jump);

	MWORD offset = Function - (MWORD)BaseAddress - 4;
	WriteMemory(BaseAddress, (PBYTE)&offset, sizeof(offset));
#endif // _WIN64
}
#pragma endregion

#pragma region 加载系统dll
void Loadmsimg32(HINSTANCE hModule)
{
	PBYTE pImageBase = (PBYTE)hModule;
	PIMAGE_DOS_HEADER pimDH = (PIMAGE_DOS_HEADER)pImageBase;
	if (pimDH->e_magic == IMAGE_DOS_SIGNATURE)
	{
		PIMAGE_NT_HEADERS pimNH = (PIMAGE_NT_HEADERS)(pImageBase + pimDH->e_lfanew);
		if (pimNH->Signature == IMAGE_NT_SIGNATURE)
		{
			PIMAGE_EXPORT_DIRECTORY pimExD = (PIMAGE_EXPORT_DIRECTORY)(pImageBase + pimNH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
			DWORD*  pName = (DWORD*)(pImageBase + pimExD->AddressOfNames);
			DWORD*  pFunction = (DWORD*)(pImageBase + pimExD->AddressOfFunctions);
			WORD*  pNameOrdinals = (WORD*)(pImageBase + pimExD->AddressOfNameOrdinals);

			wchar_t szSysDirectory[MAX_PATH + 1];
			GetSystemDirectory(szSysDirectory, MAX_PATH);

			wchar_t szDLLPath[MAX_PATH + 1];
			lstrcpy(szDLLPath, szSysDirectory);
			lstrcat(szDLLPath, TEXT("\\msimg32.dll"));

			HINSTANCE module = LoadLibrary(szDLLPath);
			if (module)
			{
				for (size_t i = 0; i < pimExD->NumberOfNames; i++)
				{
					MWORD Original = (MWORD)GetProcAddress(module, (char*)(pImageBase + pName[i]));
					if (Original)
					{
						InstallJMP(pImageBase + pFunction[pNameOrdinals[i]], Original);
					}
				}
			}
		}
	}
}
#pragma endregion

void LoadSysDll(HINSTANCE hModule)
{
	Loadmsimg32(hModule);
}
