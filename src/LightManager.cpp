#include <LightManager.h>

LightManager::LightManager(XMFLOAT4& p_cameraPosition)
{
	m_lightCBSize = CalcConstantBufferByteSize(sizeof(LightConstants));
	auto device = Application::Get().GetDevice();
	static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	static CD3DX12_RESOURCE_DESC buffer_upload = CD3DX12_RESOURCE_DESC::Buffer(m_lightCBSize);

	// upload and map
	ThrowIfFailed(device->CreateCommittedResource(
		&heap_upload,
		D3D12_HEAP_FLAG_NONE,
		&buffer_upload,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_uploadBuffer)));
	ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));

	// initialize the data
	InitializeLightCBV(p_cameraPosition);
	CopyData();
}

LightManager::~LightManager()
{
	if (m_uploadBuffer)
	{
		m_uploadBuffer->Unmap(0, nullptr);
		m_mappedData = nullptr;
	}
}

LightConstants& LightManager::GetLightConstants()
{
	return m_lightConstants;
}

void LightManager::CopyData()
{
	memcpy(m_mappedData, &m_lightConstants, sizeof(LightConstants));
}

void LightManager::InitializeLightCBV(XMFLOAT4& p_cameraPosition)
{
	// set default light constants
	m_lightConstants.CameraPosition = p_cameraPosition;

	//m_lightConstants.AmbientLightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// DEBUG purpose to have a red light
	m_lightConstants.AmbientLightColor = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	m_lightConstants.AmbientLightStrength = 0.2f;

}