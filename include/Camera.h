#pragma once
#include <DirectXMath.h>
using namespace DirectX;

class Camera
{
public:
	Camera();

	~Camera();

	void SetPosition(const XMVECTOR& position);

	void SetRotation(const XMVECTOR& rotationAxis, const float degrees);

	void SetFOV(float fov);

	void SetAspectRatio(float aspectRatio);

	void SetNearPlane(float nearPlane);

	void SetFarPlane(float farPlane);

	XMMATRIX GetViewMatrix() const;

	XMMATRIX GetProjectionMatrix() const;

private:
	XMVECTOR m_position;
	XMVECTOR m_rotationAxis;
	float m_rotationAngle; // in degrees
	float m_fov; // in degrees
	float m_aspectRatio;
	float m_nearPlane;
	float m_farPlane;
};
