#include <cstdlib>

#include "Texture.h"
#include "Application.h"
#include "CommandQueue.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include "ResourceUploadBatch.h"

void Texture::_initialize()
{
	// first in this SRV heap is for imgui
	// after that, each Texture has 3 textures: diffuse, normal, specular
	m_srvHeapOffset = m_textureIndex * 3 + 11;

	for (unsigned int i = 0; i < ResourceIndex::MAX_NO; i++)
	{
		_loadTexture(i);
	}
}

void Texture::_loadTexture(unsigned int p_internalResourceIndex)
{
	wchar_t filename[256] = L"";
	wchar_t fullpath[512] = L"";

	size_t convertedChar = 0;
	mbstowcs_s(&convertedChar, filename, 255, m_name.c_str(), 511);

	switch (p_internalResourceIndex)
	{
	case ResourceIndex::DIFFUSE:
		swprintf_s(fullpath, 511, L"textures\\%s_diffuse.dds", filename);
		_loadDDSTexture(fullpath, p_internalResourceIndex);
		break;
	case ResourceIndex::NORMAL:
		swprintf_s(fullpath, 511, L"textures\\%s_normal.jpg", filename);
		_loadWICTexture(fullpath, p_internalResourceIndex);
		break;
	case ResourceIndex::SPECULAR:
		swprintf_s(fullpath, 511, L"textures\\%s_specular.jpg", filename);
		_loadWICTexture(fullpath, p_internalResourceIndex);
		break;
	default:
		return;
	}

	_createSRV(p_internalResourceIndex);
}

void Texture::_loadDDSTexture(const wchar_t* p_fullpath, unsigned int p_internalResourceIndex)
{
	static ID3D12Device2* device = Application::Get().GetDevice().Get();
	static auto copyCommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)->GetD3D12CommandQueue().Get();
	static ResourceUploadBatch& rub = Application::Get().GetRUB();

	rub.Begin(D3D12_COMMAND_LIST_TYPE_COPY);

	ThrowIfFailed(CreateDDSTextureFromFile(device, rub, p_fullpath, &m_resources[p_internalResourceIndex]));

	auto uploadFinished = rub.End(copyCommandQueue);

	uploadFinished.wait();
}

void Texture::_loadWICTexture(const wchar_t* p_fullpath, unsigned int p_internalResourceIndex, bool p_createMissingMipmap)
{
	static ID3D12Device2* device = Application::Get().GetDevice().Get();
	static auto copyCommandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)->GetD3D12CommandQueue().Get();
	static ResourceUploadBatch& rub = Application::Get().GetRUB();

	rub.Begin(D3D12_COMMAND_LIST_TYPE_COPY);
	ThrowIfFailed(CreateWICTextureFromFile(device, rub, p_fullpath, &m_resources[p_internalResourceIndex], p_createMissingMipmap));

	auto uploadFinished = rub.End(copyCommandQueue);

	uploadFinished.wait();
}

void Texture::_createSRV(unsigned int p_internalResourceIndex)
{
	static ID3D12Device2* device = Application::Get().GetDevice().Get();
	static UINT descriptorIncrementSize = Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(Application::Get().GetSRVHeapCPUHandle(m_srvHeapOffset + p_internalResourceIndex));

	// Create shader resource view (SRV)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_resources[p_internalResourceIndex]->GetDesc().Format; // replace with texture
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_resources[p_internalResourceIndex]->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(m_resources[p_internalResourceIndex].Get(), &srvDesc, srvHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetSrvHeapStart()
{
	return Application::Get().GetSRVHeapGPUHandle(m_srvHeapOffset);
}