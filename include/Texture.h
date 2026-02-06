#pragma once
#include <string>

#include <Helpers.h>
#include <Application.h>
#include <CommandQueue.h>
#include <d3d12.h>
#include <d3dx12.h>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

enum ResourceIndex
{
	DIFFUSE = 0,
	NORMAL = 1,
	SPECULAR = 2,
	MAX_NO = 3
};

class Texture
{
public:
	Texture(const std::string& name, unsigned int p_textureIndex)
	{
		m_name = name;
		m_textureIndex = p_textureIndex;

		// load textures
		_initialize();
	}

	~Texture()
	{
	}

	const std::string& GetName() const { return m_name; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHeapStart()
	{
		return Application::Get().GetSRVHeapGPUHandle(m_srvHeapOffset);
	}

private:
	std::string m_name;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resources[ResourceIndex::MAX_NO]; //one for each RTV-mapped SRV

	unsigned int m_textureIndex = 0; // to calculate the offset in SRV heap
	unsigned int m_srvHeapOffset = 1; // default value

	void _initialize();
	void _loadTexture(unsigned int p_internalResourceIndex);

	void _loadDDSTexture(const wchar_t* p_fullpath, unsigned int p_internalResourceIndex);
	void _loadWICTexture(const wchar_t* p_fullpath, unsigned int p_internalResourceIndex, bool p_createMissingMipmap = false);

	void _createSRV(unsigned int p_internalResourceIndex);
};