#pragma once
#include <string>

#include <d3dx12.h>
#include <d3dcompiler.h>

#include <wrl.h>
using namespace Microsoft::WRL;

class Texture
{
	public:
		Texture() = default;
		Texture(const char* p_textureName, ComPtr<ID3D12Resource> p_textureResource, unsigned int p_srvDescriptorIndex)
			: textureResource(p_textureResource)
			, srvDescriptorIndex(p_srvDescriptorIndex)
		{
			textureName = p_textureName;
		}

	ComPtr<ID3D12Resource> textureResource = nullptr;
	unsigned int srvDescriptorIndex = 0;
	std::string textureName;
	// Add texture loading and management functions here
};