#pragma once
#include <CommonHeaders.h>
#include <Shader.h>
#include <Renderable.h>
#include <vector>

class Mesh
{
public:
	Mesh(const wchar_t* p_objFilePath);
	void UseShader(Shader* shader_p);
	Shader& GetShaderObjectByRef();
	ComPtr<ID3D12GraphicsCommandList2> PopulateCommandList();
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

private:
	std::vector<float> m_vertices;
	std::vector<float> m_normals;
	std::vector<uint32_t> m_triangles;
	std::vector<uint32_t> m_triangleNormalIndex;
	
	ComPtr<ID3D12GraphicsCommandList2> m_commandList;
	// Vertex buffer for the mesh.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	// Index buffer for the mesh.
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	ComPtr<ID3D12Device2> m_device; // save all these
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	// Depth buffer.
	ComPtr<ID3D12Resource> m_depthBuffer;
	// Descriptor heap for depth buffer.
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	Shader* m_shader_p;

	// 3D matrices
	float m_fov;
	DirectX::XMMATRIX m_modelMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	void _updateBufferResources();
};
