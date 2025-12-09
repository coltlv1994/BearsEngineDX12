#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include <string>

class Instance
{
public:
	Instance(const std::wstring& name)
		: m_name(name)
	{
	}

	const std::wstring& GetName() const
	{
		return m_name;
	}

	void SetName(const std::wstring& name)
	{
		m_name = name;
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

	void SetMeshClassName(const wchar_t* p_meshClassName)
	{
		wcscpy_s(m_meshClassName, 128, p_meshClassName);
	}

	const wchar_t* GetMeshClassName() const
	{
		return m_meshClassName;
	}

private:
	std::wstring m_name;
	wchar_t m_meshClassName[128];
	XMVECTOR m_position = XMVectorZero();
	XMVECTOR m_rotation = XMVectorZero();
	XMVECTOR m_scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *world* space
	void _updateModelMatrix();
};