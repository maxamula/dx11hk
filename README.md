# DirectX 11 Detour library
light directX11 hooking library
### Hooking methods
- Virtual method table hook
- Trampoline hook
### Usage example
```
typedef HRESULT(*fnPresent_t)(void* pSwapchain, UINT SyncInterval, UINT Flags);
fnPresent_t orig;

HRESULT hkPresent(void* pSwapchain, UINT SyncInterval, UINT Flags)
{
    // Do whatever you want
    return orig(pSwapchain, SyncInterval, Flags);
}

void Main()
{
    //dxhk::D3D11VMTPresentHook(hkPresent);
    orig = (fnPresent_t)dxhk::D3D11TrampolinePresentHook(hkPresent);
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
```
### Thanks to:
LDISASM - https://github.com/Nomade040/length-disassembler
