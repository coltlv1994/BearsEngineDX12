#pragma once
#include <Mesh.h>
#include <vector>
#include <queue>

class EntityManager
{
public:
	~EntityManager();
	void AddEntity(Mesh* entity_p);
	void AddEntity(const wchar_t* p_objFilePath);
	void Render(std::vector<ComPtr<ID3D12GraphicsCommandList2>>& p_commandLists);
	void Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList);

private:
	std::vector<Mesh*> m_entities;
};