#include <EntityInstance.h>
#include <Application.h>

Instance::Instance(std::string& p_name, Mesh* p_mesh, unsigned int p_textureId)
{
	m_name = p_name;
	m_mesh_p = p_mesh;
	m_textureId = p_textureId;
}

void Instance::_updateModelMatrix()
{
	m_modelMatrix = XMMatrixScalingFromVector(m_scale);
	m_modelMatrix = XMMatrixRotationRollPitchYawFromVector(m_rotation) * m_modelMatrix;
	m_modelMatrix = XMMatrixTranslationFromVector(m_position) * m_modelMatrix;
}

void Instance::Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix, CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle)
{
	textureHandle.Offset(m_textureId, Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	XMMATRIX mvpMatrix = m_modelMatrix * p_vpMatrix;
	m_mesh_p->RenderInstance(p_commandList, mvpMatrix, textureHandle);
}