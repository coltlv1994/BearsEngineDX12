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
	m_modelMatrix = XMMatrixScalingFromVector(m_scale) * XMMatrixRotationRollPitchYawFromVector(m_rotation) * XMMatrixTranslationFromVector(m_position);
}

void Instance::Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix)
{
	if (m_mesh_p == nullptr)
	{
		// allow to have an instance without a mesh assigned
		return;
	}

	auto textureHandle = m_texture_p->GetSrvHeapStart();

	p_commandList->SetGraphicsRootDescriptorTable(0, textureHandle);

	// set vertex shader input, i.e. model matrix and its inverse transpose
	// sizeof() / 4 because we are setting 32 bit constants
	VertexShaderInput vsi = {};
	vsi.mvpMatrix = m_modelMatrix * p_vpMatrix;
	vsi.tiModel = XMMatrixTranspose(XMMatrixInverse(nullptr, m_modelMatrix));

	p_commandList->SetGraphicsRoot32BitConstants(1, sizeof(vsi) / 4, &vsi, 0);

	m_mesh_p->RenderInstance(p_commandList);
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