#include "dx11hk.h"

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include "ldisasm.h"

#include <stdint.h>
#include <psapi.h>
#include <memory.h>

#include <iostream>

#define JMP_REL32_LEN 5
#define JMP_ABS64_LEN 14

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
			D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &dumSwapChain, &dumDevice, NULL, NULL);
			return *reinterpret_cast<void***>(dumSwapChain);
		}

		void ReleaseDummy()
		{
			dumDevice->Release();
			dumSwapChain->Release();
		}

		inline void JmpNear(void* from, void* to)	// x86/x64
		{
			byte jmp[JMP_REL32_LEN] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
			*reinterpret_cast<int*>(&jmp[1]) = (uint64_t)to - (uint64_t)from - 5;
			memcpy(from, jmp, JMP_REL32_LEN);
		}

		inline void JmpFar(void* from, void* to)	// x64
		{
			byte jmp[JMP_ABS64_LEN];
			*reinterpret_cast<uint16_t*>(&jmp[0]) = 0x25FF; // reverse ff 25
			*reinterpret_cast<uint32_t*>(&jmp[2]) = 0;
			*reinterpret_cast<uint64_t*>(&jmp[6]) = (uint64_t)to;
			memcpy(from, jmp, JMP_ABS64_LEN);
		}

		__nothrow void* FindPattern(const uint8_t* signature, const unsigned int length)
		{
			uint8_t* at = 0;
			DWORD oldProtection = 0;
			MEMORY_BASIC_INFORMATION mbi{};
			while ((uint64_t)at < ULLONG_MAX)
			{
				VirtualQuery(at, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
				if (mbi.State == MEM_COMMIT)
				{
					if (VirtualProtect((LPVOID)mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldProtection) == NULL)
						goto skip;
					// memmem
					for (uint64_t i = 0; i <= mbi.RegionSize - length; i++)
					{
						void* sequenceHead = (uint8_t*)mbi.BaseAddress + i;
						if (memcmp(sequenceHead, signature, length) == 0 && sequenceHead != signature)
							return sequenceHead;
					}
					VirtualProtect((LPVOID)mbi.BaseAddress, mbi.RegionSize, oldProtection, &oldProtection);
				}
				skip:
				at += mbi.RegionSize;
			}
			return NULL;
		}
		
	} // namespace Anon


	// Detected by many games
	void* D3D11VMTPresentHook(void* fnDetour)
	{
		void** VMT = CreateDummy();
		ReleaseDummy();
		uint8_t signature[20];
		memcpy(signature, VMT, 20);
		void** origVMT = (void**)FindPattern(signature, 20);
		void* origPresent = *&origVMT[8];
		*&origVMT[8] = fnDetour;
		return origPresent;
	}

	void* D3D11TrampolinePresentHook(void* fnDetour)
	{
		/*
		7FF92C2C14C0 - 48 89 5C 24 10        - mov [rsp+10],rbx
		7FF92C2C14C5 - 48 89 74 24 20        - mov [rsp+20],rsi
		7FF92C2C14CA - 55                    - push rbp
		7FF92C2C14CB - 57                    - push rdi
		7FF92C2C14CC - 41 56                 - push r14
		7FF92C2C14CE - 48 8D 6C 24 90        - lea rbp,[rsp-70]
		7FF92C2C14D3 - 48 81 EC 70010000     - sub rsp,00000170 { 368 }
		7FF92C2C14DA - 48 8B 05 B78F0C00     - mov rax,[7FF92C38A498] { (873411543) }
		7FF92C2C14E1 - 48 33 C4              - xor rax,rsp

		Present method can be hooked without handling any relative addressing instructions

		*/
		void* fnPresent = CreateDummy()[8];
		ReleaseDummy();

		long long offset = (unsigned long long)fnDetour - (unsigned long long)fnPresent;

		const uint8_t bytesToOverride = abs(offset) < INT32_MAX ? JMP_REL32_LEN : JMP_ABS64_LEN;
		uint8_t stealBytes = 0;
		uint8_t* stolenBytes = new uint8_t[bytesToOverride];	// since there is no need to process RAI, we just store the stolen bytes in an array
		while (stealBytes < bytesToOverride)
		{
			size_t instrSize = ldisasm(&((char*)fnPresent)[stealBytes], true);												// Get instruction size	
			memcpy(&stolenBytes[stealBytes], &((char*)fnPresent)[stealBytes], instrSize);
			stealBytes += instrSize;
		}
		DWORD oldProtect;
		VirtualProtect(fnPresent, stealBytes, PAGE_EXECUTE_READWRITE, &oldProtect);
		bytesToOverride == JMP_REL32_LEN ? JmpNear(fnPresent, fnDetour) : JmpFar(fnPresent, fnDetour);
		FillMemory(&((char*)fnPresent)[bytesToOverride], stealBytes - bytesToOverride, 0x90);								// nop rest of unused bytes
		VirtualProtect(fnPresent, stealBytes, oldProtect, &oldProtect);
		
		void* origPresent = VirtualAlloc(0, bytesToOverride + JMP_ABS64_LEN, MEM_COMMIT, PAGE_EXECUTE_READWRITE);			// memory space to fit stolen bytes + 10 to jmp far/near to the rest of the func
		memcpy(origPresent, stolenBytes, bytesToOverride);
		JmpFar((void*)((unsigned long long)origPresent+bytesToOverride), (void*)((unsigned long long)fnPresent + bytesToOverride));
		return origPresent;
	}
}