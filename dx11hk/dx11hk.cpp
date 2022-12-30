#include "dx11hk.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "MinHook.h"

namespace dxhk
{
	namespace
	{
		// 
		IDXGISwapChain* dumSwapChain = NULL;
		ID3D11Device* dumDevice = NULL;

		// Returns swapchain vtable ptr
		void** CreateDummy()
		{
			DXGI_SWAP_CHAIN_DESC sd{};
			sd.BufferCount = 1;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = GetForegroundWindow();
			sd.SampleDesc.Count = 1;
			sd.Windowed = TRUE;
			sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
			HRESULT a = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &dumSwapChain, &dumDevice, NULL, NULL);
			return *reinterpret_cast<void***>(dumSwapChain);
		}

		void ReleaseDummy()
		{
			dumDevice->Release();
			dumSwapChain->Release();
		}
		
	} // namespace Anon

	void* D3D11VMTPresentHook(void* fnDetour)
	{
		// TODO: copy first 20-25 bytes of dummy vtable, release dummy and pattern scan orig vtable, modify it
		return NULL;
	}

	void* D3D11TrampolinePresentHook(void* fnDetour)
	{
		void* fnPresent = CreateDummy()[8];
		void* origPresent = NULL;
		MH_Initialize();
		MH_CreateHook(fnPresent, fnDetour, &origPresent);
		MH_EnableHook(fnPresent);
		ReleaseDummy();
		return origPresent;
	}
}