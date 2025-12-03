#include <Camera.h>

Camera::Camera()
	: m_position(XMVectorZero())
	, m_rotationAxis(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f))
	, m_rotationAngle(0.0f)
	, m_fov(45.0f)
	, m_aspectRatio(16.0f / 9.0f)
	, m_nearPlane(0.1f)
	, m_farPlane(1000.0f)
{
}

Camera::~Camera()
{
}

void Camera::SetPosition(const XMVECTOR& position)
{
	m_position = position;
}

void Camera::SetRotation(const XMVECTOR& rotationAxis, const float degrees)
{
	m_rotationAxis = rotationAxis;
	m_rotationAngle = degrees;
}

void Camera::SetFOV(float fov)
{
	m_fov = fov;
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
}

void Camera::SetNearPlane(float nearPlane)
{
	m_nearPlane = nearPlane;
}

void Camera::SetFarPlane(float farPlane)
{
	m_farPlane = farPlane;
}

XMMATRIX Camera::GetViewMatrix() const
{
	XMMATRIX rotationMatrix = XMMatrixRotationAxis(m_rotationAxis, XMConvertToRadians(m_rotationAngle));
	XMVECTOR lookAt = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
	lookAt = XMVector3Normalize(lookAt);
	XMVECTOR upDirection = XMVector3TransformCoord(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);
	upDirection = XMVector3Normalize(upDirection);
	XMVECTOR focusPoint = m_position + lookAt;
	return XMMatrixLookAtLH(m_position, focusPoint, upDirection);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

