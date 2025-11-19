#pragma once
#include <Renderable.h>
#include <vector>

class EntityManager
{
public:
	EntityManager();

	void AddEntity(Renderable& entity);
	void Render();

private:
	std::vector<Renderable*> m_entities;
};