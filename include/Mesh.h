#pragma once
#include <Shader.h>
#include <vector>
#include <Helpers.h>
#include <map>
#include <string>

using namespace DirectX;

// Global function definition

class Mesh
{
public:
	bool Initialize(const wchar_t* p_objFilePath);
	void LoadOBJFile(const wchar_t* p_objFilePath);
	void UseShader(Shader* shader_p);
	void LoadDataToGPU();
	void ReadFromBinaryFile(const wchar_t* p_binFilePath);
	void WriteToBinaryFile(const wchar_t* p_binFilePath);
	void SetMeshClassName(const std::string& meshClassName);
	const std::string& GetMeshClassName();

	// Render work
	void RenderInstance(ComPtr<ID3D12GraphicsCommandList2> p_commandList);

private:
	std::string m_meshClassName;
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<float> m_texcoords;

	std::vector<uint32_t> m_triangles;
	std::vector<uint32_t> m_triangleNormalIndex;
	std::vector<uint32_t> m_triangleTexcoordIndex;

	std::vector<VertexPosColor> combinedBuffer;

	UINT m_triangleCount = 0;

	// Vertex buffer for the mesh.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	// Index buffer for the mesh.
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	Shader* m_shader_p;

	// Create a GPU buffer.
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
};
