#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <wrl.h>
using namespace Microsoft::WRL;

#include <vector>

#include "Helpers.h"
#include "Shader.h"
#include "BearWindow.h"
#include "EntityInstance.h"

class D3D12Renderer
{
public:
	D3D12Renderer() = delete;
	D3D12Renderer(const D3D12Renderer&) = delete;
	D3D12Renderer& operator=(const D3D12Renderer&) = delete;

	// Taken from Shader class; renderer will handle shaders in the plan
	D3D12Renderer(const wchar_t* p_1stVsPath, const wchar_t* p_1sPsPath, const wchar_t* p_2ndVsPath, const wchar_t* p_2ndPsPath);

	void Render(BearWindow& window);

private:
	void _transitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	// Clear a render target view.
	void _clearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	// Clear the depth of a depth-stencil view.
	void _clearDepthBuffer(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	void _renderFirstPass(ComPtr<ID3D12GraphicsCommandList2> commandList, const std::vector<Instance*>& instanceList, const XMMATRIX& vpMatrix);

	void _renderSecondPass(ComPtr<ID3D12GraphicsCommandList2> commandList, const XMMATRIX& invSPVMatrix, const RenderResource& currentRR);

	void _prepare2ndPassResources();

	uint64_t m_fenceValues[BearWindow::BufferCount] = {};
	D3D12_RECT m_scissorRect;

	Shader* m_shader_p;

	ComPtr<ID3D12Resource> m_2ndPassVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_2ndPassVertexBufferView;
};
