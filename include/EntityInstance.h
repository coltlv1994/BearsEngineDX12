#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <Mesh.h>
#include <Texture.h>

class Instance
{
public:
	Instance(std::string& p_name, Texture* p_texture_p, Mesh* p_mesh = nullptr);

	const std::string& GetName() 
	{
		return m_name;
	}

	void SetName(const std::string& name)
	{
		m_name = name;
	}

	Mesh* GetMeshClassPointer()
	{
		return m_mesh_p;
	}

	void SetMeshClassPointer(Mesh* mesh_p)
	{
		m_mesh_p = mesh_p;
	}

	XMVECTOR GetPosition() const
	{
		return m_position;
	}

	XMVECTOR GetRotation() const
	{
		return m_rotation;
	}

	XMVECTOR GetScale() const
	{
		return m_scale;
	}

	XMMATRIX GetModelMatrix() const
	{
		return m_modelMatrix;
	}

	void SetPosition(const XMVECTOR& position)
	{
		m_position = position;
		_updateModelMatrix();
	}

	void SetPosition(float x, float y, float z)
	{
		m_position = XMVectorSet(x, y, z, 0.0f);
		_updateModelMatrix();
	}

	void SetRotation(const XMVECTOR& rotation)
	{
		m_rotation = rotation;
		_updateModelMatrix();
	}

	void SetRotation(float xDegree, float yDegree, float zDegree)
	{
		m_rotation = XMVectorSet(xDegree, yDegree, zDegree, 0.0f);
		_updateModelMatrix();
	}

	void SetScale(const XMVECTOR& scale)
	{
		m_scale = scale;
		_updateModelMatrix();
	}

	void SetScale(float x, float y, float z)
	{
		m_scale = XMVectorSet(x, y, z, 0.0f);
		_updateModelMatrix();
	}

	unsigned int GetTextureId() const
	{
		return m_texture_p->srvDescriptorIndex;
	}

	std::string& GetTextureName()
	{
		return m_texture_p->textureName;
	}

	const std::string& GetMeshName()
	{
		if (m_mesh_p == nullptr)
		{
			static std::string emptyString = "<null object>";
			return emptyString;
		}
		else
		{
			return m_mesh_p->GetMeshClassName();
		}
	}

	void SetTexture(Texture* p_texture_p)
	{
		m_texture_p = p_texture_p;
	}

	void SetMeshByName(const std::string& p_meshName);

	void SetTextureByName(const std::string& p_textureName);

	void Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix, CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle);

private:
	std::string m_name;
	XMVECTOR m_position = XMVectorZero();
	XMVECTOR m_rotation = XMVectorZero();
	XMVECTOR m_scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *world* space
	Mesh* m_mesh_p = nullptr;
	Texture* m_texture_p = nullptr;

	void _updateModelMatrix();
};