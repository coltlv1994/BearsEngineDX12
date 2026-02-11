#pragma once
#include <DirectXMath.h>
using namespace DirectX;

class Camera
{
public:
	Camera();

	~Camera();

	void SetPosition(const XMVECTOR& position);
	void SetPosition(float x, float y, float z);

	void SetRotation(const XMVECTOR& rotationDegree);
	void SetRotation(float xDegree, float yDegree, float zDegree);

	void SetFOV(float fov);

	void SetAspectRatio(float aspectRatio);

	void SetNearPlane(float nearPlane);

	void SetFarPlane(float farPlane);

	XMMATRIX GetViewProjectionMatrix() const;

	XMMATRIX GetInvPVMatrix() const;

	XMVECTOR GetLookAtDirection() const;

	Camera& operator=(const Camera& other);

	XMVECTOR GetPosition() const;
	XMVECTOR GetRotation() const;

	void AddRotation(float x, float y);

private:
	XMVECTOR m_position;
	XMVECTOR m_rotation;
	float m_fov; // in degrees
	float m_aspectRatio;
	float m_nearPlane;
	float m_farPlane;
	XMVECTOR m_lookAtDirection;

	// 3D matrices
/*
* (Explanation keeps here for reference)
* The "model" matrix is one that takes your object from it's "local" space,
  and positions it in the "world" you are viewing. Generally it just
  modifies it's position, rotation and scale.

  The "view" matrix is the position and orientation of your camera that views
  the world. The inverse of this is used to take objects that are in the world,
  and move everything such that the camera is at the origin, looking down
  the Z (or -Z depending on convention) axis. This is done so that the the next steps are easier and simpler.

  The "projection" matrix is a bit special, in that it is no longer
  a simple rotate/translate, but also scales objects based on distance,
  and "pulls in" objects so that everything in the frustum is contained inside a unit-cube of space.

  NOTE: this camera class only handles the view and projection matrices; model matrices belong to individual instances.
*/

	XMMATRIX m_viewProjectionMatrix;
	XMMATRIX m_invPVMatrix;
	void _updateVPMatrix();
};
