#pragma once
#include <Mesh.h>
#include <vector>
#include <map>
#include <string>
#include <Shader.h>

class MeshManager
{
public:
	MeshManager() = default;
	~MeshManager();
	bool AddMesh(const std::wstring& meshName, Mesh* mesh_p);
	bool AddMesh(std::wstring meshPath, Shader* p_shader_p);
	Mesh* GetMesh(const std::wstring& meshName);
	bool RemoveMesh(const std::wstring& meshName);
	void ClearMeshes();

	void RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

	// for DEBUG
	void AddOneInstanceToMesh_DEBUG();

private:
	std::map<std::wstring, Mesh*> m_meshes; // map of mesh name to Mesh pointer
	std::wstring _generateMeshName(const std::wstring& meshPath);
};