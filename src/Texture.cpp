#include <Texture.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Texture::_initialize()
{
	std::string diffuseTextureName = "textures\\" + m_name + "_diffuse.jpg";
	std::string normalTextureName = "textures\\" + m_name + "_normal.jpg";
	std::string specularTextureName = "textures\\" + m_name + "_specular.jpg";

	// first in this SRV heap is for imgui
	// after that, each Texture has 3 textures: diffuse, normal, specular
	m_srvHeapOffset = m_textureIndex * 3 + 11;

	_loadTexture(diffuseTextureName, 0);
	_loadTexture(normalTextureName, 1);
	_loadTexture(specularTextureName, 2);
}

bool Texture::_loadTexture(const std::string& textureFilePath, UINT resourceIndex)
{
	// Load textures and create SRV descriptors in the heap
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	static CD3DX12_HEAP_PROPERTIES heap_default = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	unsigned char* imageData = nullptr;

	ComPtr<ID3D12Resource> texture;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	int width, height, channels;
	imageData = stbi_load(textureFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha); // force 4 channels

	if (imageData == nullptr)
	{
		// Failed to load image
		return false;
	}

	// Describe and create a Texture2D.
	textureDesc.Width = width;
	textureDesc.Height = height;

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

	// Create shader resource view (SRV)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format; // replace with texture
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SRVHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(m_srvHeapOffset + resourceIndex, Application::Get().GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
	// Store texture resource
	m_resources[resourceIndex] = texture;

	stbi_image_free(imageData);

	return true;
}