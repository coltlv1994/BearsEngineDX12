#pragma once
#include <vector>
#include <memory>

#include <Mesh.h>

class EntityManager
{
public:
	void Update(double p_deltaTime);
	void Render(ComPtr<ID3D12GraphicsCommandList2> p_commandList);
	void AddEntity(std::shared_ptr<Mesh> p_Mesh);
private:
	std::vector<std::shared_ptr<Mesh>> m_entities;
	float m_deltaTime = 0.0f;
};