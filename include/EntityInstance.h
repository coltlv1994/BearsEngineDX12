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

	XMVECTOR& GetPosition()
	{
		return m_position;
	}

	void GetRotation(XMVECTOR& outRotationAxis, float& outRotationAngle) const
	{
		outRotationAxis = m_rotationAxis;
		outRotationAngle = m_rotationAngle;
	}

	XMVECTOR& GetScale()
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

	void SetRotation(const XMVECTOR& rotationAxis, const float degrees)
	{
		m_rotationAxis = rotationAxis;
		m_rotationAngle = degrees;
		_updateModelMatrix();
	}

	void SetScale(const XMVECTOR& scale)
	{
		m_scale = scale;
		_updateModelMatrix();
	}

private:
	std::wstring m_name;
	XMVECTOR m_position = XMVectorZero();
	XMVECTOR m_rotationAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	float m_rotationAngle = 0.0f; // in degrees
	XMVECTOR m_scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *world* space
	void _updateModelMatrix();
};