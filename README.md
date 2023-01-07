# DirectX 11 Detour library
light directX11 hooking library
### Hooking methods
- Virtual method table hook
- Trampoline hook
### Usage example
```
void hkPresent()
{
    // Do whatever you want
}

void Main()
{
    // Create hook
    dxhk::D3D11VMTPresentHook(hkPresent);
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
