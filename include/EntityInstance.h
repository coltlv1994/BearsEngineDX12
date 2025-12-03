#pragma once
#include <DirectXMath.h>
using namespace DirectX;

#include <string>

class Instance
{
public:
	Instance(const std::string& name)
		: m_name(name)
	{
	}

	const std::string& GetName() const
	{
		return m_name;
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
	std::string m_name;
	XMVECTOR m_position = XMVectorZero();
	XMVECTOR m_rotationAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	float m_rotationAngle = 0.0f; // in degrees
	XMVECTOR m_scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMMATRIX m_modelMatrix = XMMatrixIdentity(); // position, rotaion, scale in *world* space
	void _updateModelMatrix();
};