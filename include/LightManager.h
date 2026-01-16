#pragma once
#include <Helpers.h>
#include <Application.h>
#include <d3dx12.h>
#include <DirectXMath.h>
using namespace DirectX;

class LightManager
{
public:
	LightManager(XMFLOAT4& p_cameraPosition);

	~LightManager();

	LightConstants& GetLightConstants();

	void CopyData(LightConstants* p_lightConstant_p);

	void UpdateCameraPosition(XMFLOAT4& p_cameraPosition)
	{
		m_lightConstants.CameraPosition = p_cameraPosition;
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetLightCBVGPUAddress()
	{
		return m_uploadBuffer->GetGPUVirtualAddress();
	}

private:
	LightConstants m_lightConstants;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
	unsigned char* m_mappedData = nullptr;

	UINT m_lightCBSize = 0;
};