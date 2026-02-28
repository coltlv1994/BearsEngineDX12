#include <EntityInstance.h>
#include <Application.h>
#include <MeshManager.h>

Instance::Instance(std::string& p_name, Texture* p_texture_p, Mesh* p_mesh)
{
	m_name = p_name;
	m_mesh_p = p_mesh;
	m_texture_p = p_texture_p;
}

const std::string& Instance::GetName()
{
	return m_name;
}

void Instance::SetName(const std::string& name)
{
	m_name = name;
}

Mesh* Instance::GetMeshClassPointer()
{
	return m_mesh_p;
}

void Instance::SetMeshClassPointer(Mesh* mesh_p)
{
	m_mesh_p = mesh_p;
}

XMVECTOR Instance::GetPosition() const
{
	return m_position;
}

XMVECTOR Instance::GetRotation() const
{
	return m_rotation;
}

XMVECTOR Instance::GetRotQuaternion() const
{
	return XMQuaternionRotationRollPitchYawFromVector(m_rotation);
}

XMVECTOR Instance::GetScale() const
{
	return m_scale;
}

XMMATRIX Instance::GetModelMatrix() const
{
	return m_modelMatrix;
}

void Instance::SetPosition(const XMVECTOR& position)
{
	m_position = position;
	_updateModelMatrix();
}

void Instance::SetPosition(float x, float y, float z)
{
	m_position = XMVectorSet(x, y, z, 1.0f);
	_updateModelMatrix();
}

void Instance::SetRotation(const XMVECTOR& rotation)
{
	m_rotation = rotation;
	_updateModelMatrix();
}

void Instance::SetRotation(float xDegree, float yDegree, float zDegree)
{
	m_rotation = XMVectorSet(xDegree, yDegree, zDegree, 0.0f);
	_updateModelMatrix();
}

void Instance::SetScale(const XMVECTOR& scale)
{
	m_scale = scale;
	_updateModelMatrix();
}

void Instance::SetScale(float x, float y, float z)
{
	m_scale = XMVectorSet(x, y, z, 0.0f);
	_updateModelMatrix();
}

const std::string Instance::GetTextureName()
{
	return m_texture_p->GetName();
}

const std::string& Instance::GetMeshName()
{
	if (m_mesh_p == nullptr)
	{
		static std::string emptyString = "null_object";
		return emptyString;
	}
	else
	{
		return m_mesh_p->GetMeshClassName();
	}
}

void Instance::SetTexture(Texture* p_texture_p)
{
	m_texture_p = p_texture_p;
}

JoltBodyShape Instance::GetBodyShape()
{
	return m_bodyShape;
}

void Instance::SetBodyShape(JoltBodyShape bodyShape)
{
	m_bodyShape = bodyShape;
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

	static std::string sphereString = "sphere";
	static std::string cubeString = "cube";

	if (p_meshName == sphereString)
	{
		SetBodyShape(JoltBodyShape::Sphere);
		
	}
	else if (p_meshName == cubeString)
	{
		SetBodyShape(JoltBodyShape::Cube);
	}
	else
	{
		SetBodyShape(JoltBodyShape::Empty);
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