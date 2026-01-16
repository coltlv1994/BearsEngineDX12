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
	CopyData(&m_lightConstants);
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

void LightManager::CopyData(LightConstants* p_lightConstant_p)
{
	memcpy(m_mappedData, p_lightConstant_p, sizeof(LightConstants));
}
