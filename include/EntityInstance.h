#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include <string>
#include <Mesh.h>
#include <Texture.h>

#include <Helpers.h>
#include <JoltHelper.h>

class Instance
{
public:
	Instance(std::string& p_name, Texture* p_texture_p, Mesh* p_mesh = nullptr);

	const std::string& GetName();

	void SetName(const std::string& name);

	Mesh* GetMeshClassPointer();

	void SetMeshClassPointer(Mesh* mesh_p);

	XMVECTOR GetPosition() const;

	XMVECTOR GetRotation() const;

	XMVECTOR GetRotQuaternion() const;

	XMVECTOR GetScale() const;

	XMMATRIX GetModelMatrix() const;

	void SetPosition(const XMVECTOR& position);

	void SetPosition(float x, float y, float z);

	void SetRotation(const XMVECTOR& rotation);

	void SetRotation(float xDegree, float yDegree, float zDegree);

	void SetScale(const XMVECTOR& scale);

	void SetScale(float x, float y, float z);

	const std::string GetTextureName();

	const std::string& GetMeshName();

	void SetTexture(Texture* p_texture_p);

	JoltBodyShape GetBodyShape();

	void SetBodyId(BodyID& bodyID);

	void SetBodyShape(JoltBodyShape bodyShape);

	void SetMeshByName(const std::string& p_meshName);

	void SetTextureByName(const std::string& p_textureName);

	void Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

private:
	std::string m_name;
	XMVECTOR m_position = XMVectorZero();
	XMVECTOR m_rotation = XMVectorZero();
	XMVECTOR m_scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *world* space
	Mesh* m_mesh_p = nullptr;
	Texture* m_texture_p = nullptr;

	BodyID m_bodyID; // for physics, start with invalid
	JoltBodyShape m_bodyShape = JoltBodyShape::Empty;

	void _updateModelMatrix();
};
