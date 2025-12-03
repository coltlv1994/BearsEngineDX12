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

}