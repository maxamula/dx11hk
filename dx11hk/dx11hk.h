#pragma once

namespace dxhk
{
	void* D3D11VMTPresentHook(void* fnDetour);
	void* D3D11TrampolinePresentHook(void* fnDetour);
}