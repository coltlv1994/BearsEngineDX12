#include <EntityInstance.h>
#include <Application.h>
#include <MeshManager.h>

Instance::Instance(std::string& p_name, Texture* p_texture_p, Mesh* p_mesh)
{
	m_name = p_name;
	m_mesh_p = p_mesh;
	m_texture_p = p_texture_p;
}

void Instance::_updateModelMatrix()
{
	m_modelMatrix = XMMatrixTranslationFromVector(m_position);
	m_modelMatrix = XMMatrixRotationRollPitchYawFromVector(m_rotation) * m_modelMatrix;
	m_modelMatrix = XMMatrixScalingFromVector(m_scale) * m_modelMatrix;
}

void Instance::Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix, CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle)
{
	if (m_mesh_p == nullptr)
	{
		// allow to have an instance without a mesh assigned
		return;
	}

	textureHandle.Offset(m_texture_p->srvDescriptorIndex, Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	XMMATRIX mvpMatrix = m_modelMatrix * p_vpMatrix;
	m_mesh_p->RenderInstance(p_commandList, mvpMatrix, textureHandle);
}

void Instance::SetMeshByName(const std::string& p_meshName)
{
	Mesh* mesh_p = MeshManager::Get().GetMeshByName(p_meshName);
	if (mesh_p)
	{
		m_mesh_p = mesh_p;
	}
	else
	{
		m_mesh_p = nullptr;
	}
}

void Instance::SetTextureByName(const std::string& p_textureName)
{
	Texture* texture_p = MeshManager::Get().GetTextureByName(p_textureName);
	if (texture_p)
	{
		m_texture_p = texture_p;
	}
}