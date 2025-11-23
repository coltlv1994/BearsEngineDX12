#pragma once
#include <Mesh.h>
#include <vector>

class EntityManager
{
public:
	EntityManager();

	void AddEntity(Mesh* entity_p);
	void Render();

private:
	std::vector<Mesh*> m_entities;
};