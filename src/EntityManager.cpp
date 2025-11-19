#include <EntityManager.h>

void EntityManager::AddEntity(Renderable& entity)
{
	m_entities.push_back(&entity);
}

void EntityManager::Render()
{
	for (auto en : m_entities)
	{
		en->Render();
	}
}