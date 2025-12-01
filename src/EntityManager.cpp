#include <EntityManager.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
using namespace DirectX;

void EntityManager::AddEntity(std::shared_ptr<Mesh> p_Mesh)
{
	m_entities.push_back(p_Mesh);
}

void EntityManager::Update(double p_deltaTime)
{
	// for now we use float
	m_deltaTime = static_cast<float>(p_deltaTime);
}

void EntityManager::Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList)
{
	for (auto mesh_p : m_entities)
	{
        // Set FOV
        float m_fov = 45.0f;
        mesh_p->SetFOV(m_fov);

        // Update the model matrix.
        XMMATRIX mScale = XMMatrixScaling(0.3f, 0.3f, 0.3f);
        const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
        XMMATRIX modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(45.0));
        XMMATRIX modelAfterScale = modelMatrix * mScale;
        mesh_p->SetModelMatrix(modelAfterScale);

        // Update the view matrix.
        const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
        const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
        const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
        XMMATRIX viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
        mesh_p->SetViewMatrix(viewMatrix);

        // Update the projection matrix.
        float aspectRatio =  1280.0 / 720.0;
        XMMATRIX projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), aspectRatio, 0.1f, 100.0f);
        mesh_p->SetProjectionMatrix(projectionMatrix);

		mesh_p->PopulateCommandList(p_commandList, m_deltaTime);
	}
}