#pragma once
#include <CommonHeaders.h>
#include <Shader.h>
#include <Renderable.h>
#include <vector>

class Mesh
{
public:
	Mesh(const wchar_t* verticesPath, const wchar_t* indicesPath);
	void UseShader(Shader* shader_p);
	Shader& GetShaderObjectByRef();
	ComPtr<ID3D12GraphicsCommandList2> PopulateCommandList();
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

private:
	std::vector<float> m_vertices;
	std::vector<uint16_t> m_indices;
	ComPtr<ID3D12GraphicsCommandList2> m_commandList;

	Shader* m_shader_p;

	void _readFromFile(const wchar_t* verticesPath, const wchar_t* indicesPath);
};
