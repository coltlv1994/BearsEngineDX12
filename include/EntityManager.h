#pragma once
#include <Renderable.h>
#include <vector>

class EntityManager
{
public:
	EntityManager();

private:
	std::vector<Renderable> m_entities;
};