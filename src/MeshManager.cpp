#include <MeshManager.h>

MeshManager::~MeshManager()
{
	ClearMeshes();
}

bool MeshManager::AddMesh(const std::wstring& meshName, Mesh* mesh_p)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return false;
	}
	m_meshes[meshName] = mesh_p;
	return true;
}

bool MeshManager::AddMesh(std::wstring meshPath, Shader* p_shader_p)
{
	std::wstring meshName = _generateMeshName(meshPath);
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return false;
	}
	else
	{
		Mesh* newMesh = new Mesh(meshPath.c_str());
		if (newMesh)
		{
			m_meshes[meshName] = newMesh;
			newMesh->UseShader(p_shader_p);
			return true;
		}
		else
		{
			return false;
		}
	}
}

Mesh* MeshManager::GetMesh(const std::wstring& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return result->second;
	}
	else
	{
		return nullptr;
	}
}

bool MeshManager::RemoveMesh(const std::wstring& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		delete result->second;
		m_meshes.erase(result->first);
	}
	else
	{
		return false;
	}
}

void MeshManager::ClearMeshes()
{
	for (auto mesh_p : m_meshes)
	{
		delete mesh_p.second;
	}

	m_meshes.clear();
}

std::wstring MeshManager::_generateMeshName(const std::wstring& meshPath)
{
	size_t foundBackslash = meshPath.find_last_of(L"\\");
	std::wstring filename = meshPath.substr(foundBackslash + 1);
	size_t foundDot = filename.find_first_of(L".");
	std::wstring fileNameWithoutDot = filename.substr(0, foundDot);
	return fileNameWithoutDot;
}

void MeshManager::RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix)
{
	for (auto meshClass : m_meshes)
	{
		meshClass.second->RenderInstances(p_commandList, p_vpMatrix);
	}
}

void MeshManager::AddOneInstanceToMesh_DEBUG()
{
	for (auto& item : m_meshes)
	{
		if (item.second->AddInstance())
		{
			return;
		}
	}
}