#define PSAPI_VERSION 1
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>
#include <psapi.h>
#include "dinput8.h"

DirectInput8CreateProc m_pDirectInput8Create;
DllCanUnloadNowProc m_pDllCanUnloadNow;
DllGetClassObjectProc m_pDllGetClassObject;
DllRegisterServerProc m_pDllRegisterServer;
DllUnregisterServerProc m_pDllUnregisterServer;
GetdfDIJoystickProc m_pGetdfDIJoystick;

PCHAR FindPattern(LPCSTR pattern, LPCSTR mask, PCHAR start, DWORD size)
{
    DWORD _;
    VirtualProtect((PVOID)start, size, PAGE_EXECUTE_READWRITE, &_);

    size_t patternLength = strlen(pattern);

    for (size_t i = 0; i < size - patternLength; ++i)
    {
        bool found = true;

        for (size_t j = 0; j < patternLength; ++j)
        {
            if (mask[j] != '?' && pattern[j] != *(start + i + j))
            {
                found = false;
                break;
            }
        }

        if (found)
        {
            return (start + i);
        }
    }

    return NULL;
}

HMODULE LoadSystemModule(const char* dllName)
{
    char path[MAX_PATH];
    GetSystemDirectoryA(path, MAX_PATH);
    PathAppendA(path, dllName);

    return LoadLibraryA(path);
}

void SetDinputFuncs(HMODULE dinputModule)
{
    m_pDirectInput8Create = (DirectInput8CreateProc)GetProcAddress(dinputModule, "DirectInput8Create");
    m_pDllCanUnloadNow = (DllCanUnloadNowProc)GetProcAddress(dinputModule, "DllCanUnloadNow");
    m_pDllGetClassObject = (DllGetClassObjectProc)GetProcAddress(dinputModule, "DllGetClassObject");
    m_pDllRegisterServer = (DllRegisterServerProc)GetProcAddress(dinputModule, "DllRegisterServer");
    m_pDllUnregisterServer = (DllUnregisterServerProc)GetProcAddress(dinputModule, "DllUnregisterServer");
    m_pGetdfDIJoystick = (GetdfDIJoystickProc)GetProcAddress(dinputModule, "GetdfDIJoystick");
}

void ApplyWindowedFix(HMODULE dinputModule)
{
    MODULEINFO moduleInfo = { 0 };

    if (GetModuleInformation(GetCurrentProcess(), dinputModule, &moduleInfo, sizeof(MODULEINFO)) == FALSE)
    {
        // If the module information can't be retrieved for whatever reason, don't proceed
        return;
    }

    // 01
    // 74 ??
    // 8D ?? 08 (start the jump here)
    // 50
    // FF 15 ?? ?? ?? ??
    // 8D 45 ??
    // 50
    // FF ?? 10
    // FF 15 ?? ?? ?? ??
    // 8B 45 ??
    PCHAR jmpStart = FindPattern(
            "\x01\x74?\x8D?\x08\x50\xFF\x15????\x8D\x45?\x50\xFF?\x10\xFF\x15????\x8B\x45?",
            "xx?x?xxxx????xx?xx?xxx????xx?",
            (PCHAR)moduleInfo.lpBaseOfDll, moduleInfo.SizeOfImage);

    if (jmpStart == NULL)
    {
        // If it can't find the first pattern, don't bother
        return;
    }

    jmpStart += 3; // Set offset after the je

    // D1 ??
    // ??
    // FF 15 ?? ?? ?? ??
    // 53 (jump to here, right after the SetCursorPos call)
    // FF 15 ?? ?? ?? ??
    // 8B ?? 14
    PCHAR jmpDest = FindPattern(
            "\xD1??\xFF\x15????\x53\xFF\x15????\x8B?\x14",
            "x??xx????xxx????x?x",
            jmpStart, 100);

    if (jmpDest == NULL)
    {
        // If the second pattern can't be found within 100 bytes after the first pattern, try an alternative pattern
        // D1 ??
        // ??
        // FF 15 ?? ?? ?? ??
        // 33 DB (jump to here, right after the SetCursorPos call)
        // 53
        // FF 15 ?? ?? ?? ??
        // 8B ?? 14
        jmpDest = FindPattern(
                "\xD1??\xFF\x15????\x33\xDB\x53\xFF\x15????\x8B?\x14",
                "x??xx????xxxxx????x?x",
                jmpStart, 100);

        if (jmpDest == NULL)
        {
            // If the alternative pattern can't be found either, give up
            return;
        }
    }

    jmpDest += 9; // Set offset at push ebx

    DWORD _;
    VirtualProtect((PVOID)jmpStart, sizeof(BYTE) * 2, PAGE_EXECUTE_READWRITE, &_);
    *(PBYTE)jmpStart = 0xEB; // jmp instruction
    *(PCHAR)(jmpStart + 1) = ((char)(jmpDest - (jmpStart + 2))); // jump to the destination;
    // (jmpStart + 2 because the jump distance is counted starting two bytes after the je instruction)
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    static HMODULE dinputModule = NULL;

    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(lpReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            dinputModule = LoadSystemModule("dinput8.dll");

            if (!dinputModule)
            {
                // Uhh...
                return FALSE;
            }

            SetDinputFuncs(dinputModule);
            ApplyWindowedFix(dinputModule);

            break;
        case DLL_PROCESS_DETACH:
            FreeLibrary(dinputModule);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
    if (!m_pDirectInput8Create)
        return E_FAIL;

    return m_pDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

HRESULT WINAPI DllCanUnloadNow()
{
    if (!m_pDllCanUnloadNow)
        return E_FAIL;

    return m_pDllCanUnloadNow();
}

HRESULT WINAPI DllGetClassObject(IN REFCLSID rclsid, IN REFIID riid, OUT LPVOID FAR* ppv)
{
    if (!m_pDllGetClassObject)
        return E_FAIL;

    return m_pDllGetClassObject(rclsid, riid, ppv);
}

HRESULT WINAPI DllRegisterServer()
{
    if (!m_pDllRegisterServer)
        return E_FAIL;

    return m_pDllRegisterServer();
}

HRESULT WINAPI DllUnregisterServer()
{
    if (!m_pDllUnregisterServer)
        return E_FAIL;

    return m_pDllUnregisterServer();
}

LPCDIDATAFORMAT WINAPI GetdfDIJoystick()
{
    if (!m_pGetdfDIJoystick)
        return NULL;

    return m_pGetdfDIJoystick();
}
