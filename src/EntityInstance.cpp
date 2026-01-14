#include <EntityInstance.h>
#include <Application.h>
#include <MeshManager.h>

Instance::Instance(std::string& p_name, Texture* p_texture_p, Material* p_material_p, Mesh* p_mesh)
{
	m_name = p_name;
	m_mesh_p = p_mesh;
	m_texture_p = p_texture_p;
	m_material_p = p_material_p;
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

	static UINT descriptorSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureHandle.Offset(m_texture_p->srvDescriptorIndex, descriptorSize);
	p_commandList->SetGraphicsRootDescriptorTable(0, textureHandle);

	// set vertex shader input, i.e. model matrix and its inverse transpose
    // sizeof() / 4 because we are setting 32 bit constants
	VertexShaderInput vsi = {};
	vsi.mvpMatrix = m_modelMatrix * p_vpMatrix;
	vsi.t_i_modelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, m_modelMatrix));
	p_commandList->SetGraphicsRoot32BitConstants(1, sizeof(vsi) / 4, &vsi, 0);

	// material
	p_commandList->SetGraphicsRootConstantBufferView(2, m_material_p->GetMaterialCBVGPUAddress());	

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