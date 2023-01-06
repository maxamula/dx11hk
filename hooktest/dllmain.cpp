﻿// dllmain.cpp : Определяет точку входа для приложения DLL.
#include <windows.h>
#include <stdio.h>
#include "dx11hk.h"

#pragma comment(lib, "dx11hk.lib")

void hkPresent()
{
    Beep(1000, 500);
}

void Main()
{
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    dxhk::D3D11VMTPresentHook(hkPresent);
    //dxhk::D3D11TrampolinePresentHook(hkPresent);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Main, NULL, NULL, NULL));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

