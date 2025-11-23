#include <EntityManager.h>

#include <Application.h>

#include <vector>

void EntityManager::AddEntity(Mesh* entity)
{
	m_entities.push_back(entity);
}

void EntityManager::Render()
{
	Application& app = Application::GetInstance();

	// create command list for each entity
	std::vector<ComPtr<ID3D12GraphicsCommandList2>> commandLists;

	for (auto& en : m_entities)
	{
		commandLists.push_back(en->PopulateCommandList());
	}
}