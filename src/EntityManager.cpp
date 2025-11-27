#include <EntityManager.h>

#include <Application.h>

#include <vector>

EntityManager::~EntityManager()
{
	for (Mesh* mesh : m_entities)
	{
		delete mesh;
	}
}

void EntityManager::AddEntity(Mesh* entity)
{
	m_entities.push_back(entity);
}

void EntityManager::AddEntity(const wchar_t* p_objFilePath)
{
	//std::vector<Mesh*> m_entities;

	Mesh* newMesh = new Mesh(p_objFilePath);
	AddEntity(newMesh);
}

void EntityManager::Render(std::vector<ComPtr<ID3D12GraphicsCommandList2>>& p_commandLists)
{
	for (auto& en : m_entities)
	{
		p_commandLists.push_back(en->PopulateCommandList());
	}
}

void EntityManager::Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList)
{
	for (auto& en : m_entities)
	{
		en->PopulateCommandList(p_commandList);
	}
}