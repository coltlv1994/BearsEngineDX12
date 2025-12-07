#pragma once
#include <Mesh.h>
#include <vector>
#include <map>
#include <string>
#include <Shader.h>
#include <MessageQueue.h>

class MeshManager
{
public:
	static MeshManager& Get();

	static void Destroy();

	MeshManager() = default;
	~MeshManager();

	bool AddMesh(const wchar_t* meshName, Shader* p_shader_p, const wchar_t* texturePath = nullptr);
	Mesh* GetMesh(const std::wstring& meshName);
	bool RemoveMesh(const std::wstring& meshName);
	void ClearMeshes();

	void RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

	// for DEBUG
	void AddOneInstanceToMesh_DEBUG();

	void StartListeningThread()
	{
		std::thread listenerThread(&MeshManager::Listen, this);
		listenerThread.detach();
	}

	// Receive message from other systems
	void ReceiveMessage(Message* msg);

	void SetDefaultShader(Shader* shader_p)
	{
		m_defaultShader_p = shader_p;
	}


private:
	std::map<std::wstring, Mesh*> m_meshes; // map of mesh name to Mesh pointer
	std::wstring _generateMeshName(const std::wstring& meshPath);
	void _processMessage(Message& msg);
	// Message Queue access
	void Listen();
	MessageQueue m_messageQueue;
	Shader* m_defaultShader_p = nullptr;
};