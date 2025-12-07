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
	_updateVPMatrix();
}

Camera::~Camera()
{
}

void Camera::SetPosition(const XMVECTOR& position)
{
	m_position = position;
	_updateVPMatrix();
}

void Camera::SetPosition(float x, float y, float z)
{
	m_position = XMVectorSet(x, y, z, 0.0f);
	_updateVPMatrix();
}

void Camera::SetRotation(const XMVECTOR& rotationAxis, const float degrees)
{
	m_rotationAxis = rotationAxis;
	m_rotationAngle = degrees;
	_updateVPMatrix();
}

void Camera::SetRotation(float x, float y, float z, float degrees)
{
	m_rotationAxis = XMVectorSet(x, y, z, 0.0f);
	m_rotationAngle = degrees;
	_updateVPMatrix();
}

void Camera::SetFOV(float fov)
{
	m_fov = fov;
	_updateVPMatrix();
}

void Camera::SetAspectRatio(float aspectRatio)
{
	m_aspectRatio = aspectRatio;
	_updateVPMatrix();
}

void Camera::SetNearPlane(float nearPlane)
{
	m_nearPlane = nearPlane;
}

void Camera::SetFarPlane(float farPlane)
{
	m_farPlane = farPlane;
}

void Camera::_updateVPMatrix()
{
	XMMATRIX rotationMatrix = XMMatrixRotationAxis(m_rotationAxis, XMConvertToRadians(m_rotationAngle));
	XMVECTOR lookAt = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
	lookAt = XMVector3Normalize(lookAt);
	XMVECTOR upDirection = XMVector3TransformCoord(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);
	upDirection = XMVector3Normalize(upDirection);
	XMVECTOR focusPoint = m_position + lookAt;

	XMMATRIX viewMatrix = XMMatrixLookAtLH(m_position, focusPoint, upDirection);
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);

	m_viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);
}

XMMATRIX Camera::GetViewProjectionMatrix() const
{
	return m_viewProjectionMatrix;
}

