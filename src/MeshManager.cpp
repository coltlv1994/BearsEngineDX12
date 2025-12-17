#include <MeshManager.h>
#include <chrono>
#include <thread>
#include <UIManager.h>
#include <Application.h>
#include <CommandQueue.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// UIManager singleton instance
static MeshManager* gs_pSingleton = nullptr;

MeshManager& MeshManager::Get()
{
	if (gs_pSingleton == nullptr)
	{
		gs_pSingleton = new MeshManager();
	}
	return *gs_pSingleton;
}

void MeshManager::Destroy()
{
	delete gs_pSingleton;
}

MeshManager::~MeshManager()
{
	ClearMeshes();
}

bool MeshManager::AddMesh(const std::string& meshName, Shader* p_shader_p)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		return false;
	}
	else
	{
		Mesh* newMesh = new Mesh();

		wchar_t meshNameWChar[128];
		mbstowcs_s(nullptr, meshNameWChar, 128, meshName.c_str(), _TRUNCATE);

		wchar_t meshFullPath[128];
		swprintf_s(meshFullPath, L"meshes\\%s.obj", meshNameWChar);

		bool createResult = false;
		createResult = newMesh->Initialize(meshFullPath);

		if (createResult)
		{
			m_meshes[meshName] = newMesh;
			newMesh->UseShader(p_shader_p);
			newMesh->SetMeshClassName(meshName);
			return true;
		}
		else
		{
			return false;
		}
	}
}

bool MeshManager::RemoveMesh(const std::string& meshName)
{
	auto result = m_meshes.find(meshName);
	if (result != m_meshes.end())
	{
		// Mesh with the same name already exists
		delete result->second;
		m_meshes.erase(result->first);
		return true;
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

// TODO: need change
void MeshManager::RenderAllMeshes(ComPtr<ID3D12GraphicsCommandList2> p_commandList, const XMMATRIX& p_vpMatrix)
{
	for (Instance* instance_p : m_instanceList)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE textureHandle(m_SRVHeap->GetGPUDescriptorHandleForHeapStart());

		// call mesh class to render it
		instance_p->Render(p_commandList, p_vpMatrix, textureHandle);
	}
}

void MeshManager::Listen()
{
	while (true)
	{
		Message msg;
		if (m_messageQueue.PopMessage(msg))
		{
			// Process message
			_processMessage(msg);
			continue;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Avoid busy waiting
	}
}

void MeshManager::_processMessage(Message& msg)
{
	// message data is mesh name in char*
	size_t dataSize = msg.GetSize();

	switch (msg.type)
	{
	case MSG_TYPE_LOAD_MESH:
	{
		std::string meshName = std::string((char*)msg.GetData());

		Mesh* mesh_p = GetMeshByName(meshName);
		if (mesh_p)
		{
			// Mesh already exists, send back load success message
			_sendMeshLoadSuccessMessage(meshName);
			break;
		}

		// TODO: use different shader and texture if specified in message
		// texturePath is now fixed in Mesh constructor
		if (AddMesh(meshName, m_defaultShader_p))
		{
			// Successfully added mesh
			// Send back load success message
			_sendMeshLoadSuccessMessage(meshName);
		}
		else
		{
			// Failed to add mesh, send error message
			_sendMeshLoadFailedMessage(meshName);

		}
		break;
	}
	case MSG_TYPE_CREATE_INSTANCE:
	{
		std::string instanceName = "instance_" + std::to_string(m_createdInstanceCount);

		Instance* createdInstance = new Instance(instanceName, m_defaultTexture_p);
		m_instanceList.push_back(createdInstance);
		_sendInstanceReplyMessage(createdInstance);

		m_createdInstanceCount += 1;

		break;
	}
	case MSG_TYPE_RELOAD_MESH:
	{
		char* buffer = new char[dataSize];
		memcpy(buffer, msg.GetData(), dataSize);
		ReloadInfo& reloadInfo = *(ReloadInfo*)buffer;

		std::string meshName(reloadInfo.meshName);

		// try get, then create, if fail, return
		Mesh* mesh_p = GetMeshByName(meshName);
		bool result = true;
		if (!mesh_p)
		{
			result = AddMesh(meshName, m_defaultShader_p);
			if (!result)
			{
				// Failed to add mesh, send error message
				_sendMeshLoadFailedMessage(meshName);
				delete[] buffer;
				break;
			}
			mesh_p = GetMeshByName(meshName);
		}
		_sendMeshLoadSuccessMessage(meshName);

		InstanceInfo* instanceInfos_p = &reloadInfo.instanceInfos;
		for (size_t i = 0; i < reloadInfo.numOfInstances; ++i)
		{
			std::string instanceName = "instance_" + std::to_string(m_instanceList.size());
			Instance* instance_p = new Instance(instanceName, m_defaultTexture_p);
			if (instance_p)
			{
				instance_p->SetPosition(instanceInfos_p[i].position[0], instanceInfos_p[i].position[1], instanceInfos_p[i].position[2]);
				// rotation is in radians already
				instance_p->SetRotation(XMVectorSet(instanceInfos_p[i].rotation[0], instanceInfos_p[i].rotation[1], instanceInfos_p[i].rotation[2], 0.0f));
				instance_p->SetScale(XMVectorSet(instanceInfos_p[i].scale[0], instanceInfos_p[i].scale[1], instanceInfos_p[i].scale[2], 0.0f));

				m_instanceList.push_back(instance_p);
				_sendInstanceReplyMessage(instance_p);
			}
			else
			{
				// Failed to add instance, send error message
				_sendInstanceFailedMessage(meshName);
			}
		}

		// TODO: clean up unused mesh
		break;
	}
	case MSG_TYPE_LOAD_TEXTURE:
	{
		std::string textureName = std::string((char*)msg.GetData());
		bool result = false;
		// check if texture already loaded

		auto texture = m_textureMap.find(textureName);
		if (texture != m_textureMap.end())
		{
			// texture with the same name already exists
			result = false;
		}
		else
		{
			result = ReadAndUploadTexture(textureName.c_str());
		}

		// send back texture load message
		Message* msgReply = new Message();
		if (result)
		{
			msgReply->type = MSG_TYPE_TEXTURE_SUCCESS;
		}
		else
		{
			// failed to load texture, send back default texture name
			msgReply->type = MSG_TYPE_TEXTURE_FAILED;
		}
		msgReply->SetData(textureName.c_str(), textureName.length() + 1); // SetData is always copying
		UIManager::Get().ReceiveMessage(msgReply);
		break;
	}
	case MSG_TYPE_REMOVE_INSTANCE:
	{
		// data is instance pointer
		if (dataSize != POINTER_SIZE)
		{
			// invalid data size
			break;
		}
		Instance* instanceToRemove = *(Instance**)(msg.GetData());
		auto iter = std::find(m_instanceList.begin(), m_instanceList.end(), instanceToRemove);
		if (iter != m_instanceList.end())
		{
			delete *iter;
			m_instanceList.erase(iter);
		}
		break;
	}
	case MSG_TYPE_CLEAN_MESHES:
	{
		// Clean all meshes
		CleanForLoad();
		break;
	}
	default:
		break;
	}

	msg.Release();
}

// Receive message from other systems
void MeshManager::ReceiveMessage(Message* msg)
{
	m_messageQueue.PushMessage(msg);
}

void MeshManager::_sendMeshLoadSuccessMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_LOAD_SUCCESS;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceReplyMessage(Instance* createdInstance)
{
	std::string instanceName = createdInstance->GetName();
	const char* instanceNameCStr = instanceName.c_str();
	const size_t instanceNameLength = instanceName.length();

	size_t dataLength = instanceName.length() + 1 + POINTER_SIZE; // one for null terminator

	unsigned char* replyDataBuffer = new unsigned char[dataLength];
	memcpy(replyDataBuffer, instanceNameCStr, instanceNameLength + 1);
	memcpy(replyDataBuffer + instanceNameLength + 1, &createdInstance, POINTER_SIZE);

	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_INSTANCE_REPLY;
	msgReply->SetData(replyDataBuffer, dataLength); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
	delete[] replyDataBuffer;
}

void MeshManager::CleanForLoad()
{
	// clean command queue
	Application::Get().Flush();

	// clean all instances
	for (Instance* instance_p : m_instanceList)
	{
		delete instance_p;
	}
	m_instanceList.clear();

	//while (m_meshes.begin() != m_meshes.end())
	//{
	//	auto meshIter = m_meshes.begin();
	//	delete meshIter->second;
	//	m_meshes.erase(meshIter);
	//}
}

void MeshManager::_sendMeshLoadFailedMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_MESH_LOAD_FAILED;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::_sendInstanceFailedMessage(const std::string& meshName)
{
	Message* msgReply = new Message();
	msgReply->type = MSG_TYPE_INSTANCE_FAILED;
	msgReply->SetData(meshName.c_str(), meshName.length() + 1); // SetData is always copying
	UIManager::Get().ReceiveMessage(msgReply);
}

void MeshManager::CreateDefaultTexture()
{
	ReadAndUploadTexture();
	m_defaultTexture_p = m_textureMap["default_white"];
}

bool MeshManager::ReadAndUploadTexture(const char* textureName)
{
	ComPtr<ID3D12Resource> texture = nullptr;

	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	static CD3DX12_HEAP_PROPERTIES heap_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	unsigned char* imageData = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	if (textureName == nullptr)
	{
		// load default white texture
		textureDesc.Width = 128;
		textureDesc.Height = 128;

		size_t bufferSize = textureDesc.Width * textureDesc.Height * 4; // 4 channels RGBA

		imageData = new unsigned char[bufferSize];
		memset(imageData, 0xFF, bufferSize); // white texture
	}
	else
	{
		// get texture path, convert to wchar_t*
		char texturePath[500];
		sprintf_s(texturePath, "textures\\%s", textureName);

		int width, height, channels;
		imageData = stbi_load(texturePath, &width, &height, &channels, STBI_rgb_alpha); // force 4 channels

		if (imageData == nullptr)
		{
			// Failed to load image
			return false;
		}

		// Describe and create a Texture2D.
		textureDesc.Width = width;
		textureDesc.Height = height;
	}

	ThrowIfFailed(device->CreateCommittedResource(
		&heap_default,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&texture)));

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);

	static CD3DX12_HEAP_PROPERTIES heap_upload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC buffer_default_size = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
	ComPtr<ID3D12Resource> textureUploadHeap = nullptr;

	// Create the GPU upload buffer.
	ThrowIfFailed(device->CreateCommittedResource(
		&heap_upload,
		D3D12_HEAP_FLAG_NONE,
		&buffer_default_size,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureUploadHeap)));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = imageData;
	textureData.RowPitch = textureDesc.Width * 4;
	textureData.SlicePitch = textureData.RowPitch * textureDesc.Height;

	UpdateSubresources(commandList.Get(), texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

	// wait until upload is complete
	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	// The data is guaranteed to be uploaded after WaitForFenceValue()
	if (textureName == nullptr)
	{
		delete[] imageData;
	}
	else
	{
		stbi_image_free(imageData);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format; // replace with texture
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// store in from 2nd heap to avoid error with imgui
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
	UINT descriptorIndex = m_textureMap.size() + 1;
	hDescriptor.Offset(descriptorIndex, Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);

	// store texture in map
	textureName = textureName ? textureName : "default_white";
	m_textureMap[textureName] = new Texture(textureName, texture, descriptorIndex);

	return true;
}

Texture* MeshManager::GetTextureByName(const std::string& textureName)
{
	if (auto textureIter = m_textureMap.find(textureName); textureIter != m_textureMap.end())
	{
		return textureIter->second;
	}
	else
	{
		return nullptr;
	}
}

Mesh* MeshManager::GetMeshByName(const std::string& meshName)
{
	if (auto meshIter = m_meshes.find(meshName); meshIter != m_meshes.end())
	{
		return meshIter->second;
	}
	else
	{
		return nullptr;
	}
}