#pragma once
#include <Mesh.h>
#include <vector>

class MeshManager
{
public:
	MeshManager() = default;

	~MeshManager()
	{
		for (auto mesh : m_meshList)
		{
			delete mesh;
		}
		m_meshList.clear();
	}

	void AddMesh(Mesh* mesh)
	{
		m_meshList.push_back(mesh);
	}

	const std::vector<Mesh*>& GetMeshList() const
	{
		return m_meshList;
	}

	bool AddMesh(const wchar_t* p_objFilePath, Shader* shader_p)
	{
		Mesh* newMesh = new Mesh(p_objFilePath);
		if (newMesh && shader_p)
		{
			newMesh->UseShader(shader_p);
			m_meshList.push_back(newMesh);
			return true;
		}
		return false;
	}

	bool RemoveMesh(Mesh* mesh)
	{
		auto it = std::find(m_meshList.begin(), m_meshList.end(), mesh);
		if (it != m_meshList.end())
		{
			delete *it;
			m_meshList.erase(it);
			return true;
		}
		return false;
	}
private:
	std::vector<Mesh*> m_meshList;

};