#pragma once
#include <Mesh.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <Shader.h>
#include <MessageQueue.h>
#include <EntityInstance.h>
#include <Texture.h>
#include <Material.h>
#include <LightManager.h>

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

	void RenderAllMeshes2ndPass(ComPtr<ID3D12GraphicsCommandList2> p_commandList, CD3DX12_CPU_DESCRIPTOR_HANDLE p_firstPassRTVs);

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

	Material* GetMaterialByName(const std::string& materialName);

	Mesh* GetMeshByName(const std::string& meshName);

	void CreateDefaultMaterial();

	void InitializeLightManager(XMFLOAT4& p_mainCameraLocation);

	void ClearMaterials()
	{
		for (auto materialPair : m_materialMap)
		{
			delete materialPair.second;
		}
		m_materialMap.clear();
	}

private:
	std::unordered_map<std::string, Mesh*> m_meshes; // map of mesh name to Mesh pointer
	MessageQueue m_messageQueue;
	Shader* m_defaultShader_p = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap; // passed from Editor class, holds textures
	std::vector<Instance*> m_instanceList; // list of all created instances
	std::unordered_map<std::string, Texture*> m_textureMap; // map of texture file path to texture resource
	std::unordered_map<std::string, Material*> m_materialMap; // map of material name to material
	LightManager* m_lightManager_p = nullptr;

	unsigned int m_createdInstanceCount = 0; // for unique instance naming, this value should never decrease except map reloading

	uint64_t m_memInfo[2] = { 0 }; // used and total memory info

	void _processMessage(Message& msg);
	// Message Queue access
	void Listen();
	void _sendMeshLoadSuccessMessage(const std::string& meshName);
	void _sendInstanceReplyMessage(Instance* createdInstance);
	void _sendMeshLoadFailedMessage(const std::string& meshName);
	void _sendInstanceFailedMessage(const std::string& meshName);
};