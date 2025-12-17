#pragma once
#include <Mesh.h>
#include <vector>
#include <map>
#include <string>
#include <Shader.h>
#include <MessageQueue.h>
#include <EntityInstance.h>
#include <Texture.h>

class MeshManager
{
public:
	static MeshManager& Get();

	static void Destroy();

	MeshManager() = default;
	~MeshManager();

	bool AddMesh(const std::string& meshName, Shader* p_shader_p);
	bool RemoveMesh(const std::string& meshName);
	void ClearMeshes();

	void RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix);

	void StartListeningThread()
	{
		std::thread m_listenerThread(&MeshManager::Listen, this);
		m_listenerThread.detach();
	}

	// Receive message from other systems
	void ReceiveMessage(Message* msg);

	void SetDefaultShader(Shader* shader_p)
	{
		m_defaultShader_p = shader_p;
	}

	void SetSRVHeap(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap)
	{
		m_SRVHeap = srvHeap;
	}

	void CreateDefaultTexture();

	void CleanForLoad();

	bool ReadAndUploadTexture(const char* textureFilePath = nullptr);

	Texture* GetTextureByName(const std::string& textureName);

	Mesh* GetMeshByName(const std::string& meshName);

private:
	std::map<std::string, Mesh*> m_meshes; // map of mesh name to Mesh pointer
	MessageQueue m_messageQueue;
	Shader* m_defaultShader_p = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap; // passed from Editor class, holds textures
	std::vector<Instance*> m_instanceList; // list of all created instances
	std::map<std::string, Texture*> m_textureMap; // map of texture file path to texture resource

	unsigned int m_createdInstanceCount = 0; // for unique instance naming, this value should never decrease except map reloading

	Texture* m_defaultTexture_p;

	void _processMessage(Message& msg);
	// Message Queue access
	void Listen();
	void _sendMeshLoadSuccessMessage(const std::string& meshName);
	void _sendInstanceReplyMessage(Instance* createdInstance);
	void _sendMeshLoadFailedMessage(const std::string& meshName);
	void _sendInstanceFailedMessage(const std::string& meshName);
};