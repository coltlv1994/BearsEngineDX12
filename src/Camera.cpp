#include <Camera.h>

Camera::Camera()
	:m_rotation(XMVectorZero())
	, m_fov(90.0f)
	, m_aspectRatio(16.0f / 9.0f)
	, m_nearPlane(0.1f)
	, m_farPlane(1000.0f)
{
	m_position = XMVectorSet(0.0f, 0.0f, -10.0f, 1.0f);
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
	m_position = XMVectorSet(x, y, z, 1.0f);
	_updateVPMatrix();
}

void Camera::SetRotation(const XMVECTOR& rotationDegrees)
{
	m_rotation = rotationDegrees;
	_updateVPMatrix();
}

void Camera::SetRotation(float xDegree, float yDegree, float zDegree)
{
	m_rotation = XMVectorSet(xDegree, yDegree, zDegree, 0.0f);
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

XMVECTOR Camera::GetPosition() const
{
	return m_position;
}

XMVECTOR Camera::GetRotation() const
{
	return m_rotation;
}

void Camera::AddRotation(float x, float y)
{
	m_rotation = XMVectorAdd(m_rotation, XMVectorSet(x, y, 0.0f, 0.0f));
	_updateVPMatrix();
}

void Camera::_updateVPMatrix()
{
	//x - axis(pitch), then y - axis(yaw), and then z - axis(roll)
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYawFromVector(m_rotation);
	m_frontDirection = XMVector3Normalize(XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix));
	m_upDirection = XMVector3Normalize(XMVector3TransformCoord(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix));

	XMMATRIX viewMatrix = XMMatrixLookToLH(m_position, m_frontDirection, m_upDirection);
	XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);

	m_viewProjectionMatrix = XMMatrixMultiply(viewMatrix, projectionMatrix);

	m_invPVMatrix = XMMatrixInverse(nullptr, m_viewProjectionMatrix);
}

XMMATRIX Camera::GetViewProjectionMatrix() const
{
	return m_viewProjectionMatrix;
}

XMMATRIX Camera::GetInvPVMatrix() const
{
	return m_invPVMatrix;
}

Camera& Camera::operator=(const Camera& other)
{
	if (this != &other)
	{
		m_position = other.m_position;
		m_rotation = other.m_rotation;
		m_fov = other.m_fov;
		m_aspectRatio = other.m_aspectRatio;
		m_nearPlane = other.m_nearPlane;
		m_farPlane = other.m_farPlane;
		m_viewProjectionMatrix = other.m_viewProjectionMatrix;
	}
	return *this;
}

XMVECTOR Camera::GetFrontDirection() const
{
	return m_frontDirection;
}