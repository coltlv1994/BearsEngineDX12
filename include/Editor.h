#pragma once
#include <Window.h>
#include <Camera.h>

#include <DirectXMath.h>

#include <Shader.h>

#include <memory>
#include <utility>

class Editor : public std::enable_shared_from_this<Editor>
{
public:
    Editor(const std::wstring& name, int width, int height, bool vSync = true);

    bool Initialize();

    /**
     *  Load content required for the demo.
     */
    bool LoadContent();

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    void UnloadContent();

    D3D12_VIEWPORT& GetViewport();

    D3D12_RECT& GetScissorRect();

    /**
     *  Update the game logic.
     */
    void OnUpdate(UpdateEventArgs& e);

    /**
     *  Render stuff.
     */
    void OnRender(RenderEventArgs& e);

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    void OnKeyPressed(KeyEventArgs& e);

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
    void OnMouseWheel(MouseWheelEventArgs& e);


    void OnResize(ResizeEventArgs& e);

    int GetClientWidth() const
    {
        return m_width;
    }

    int GetClientHeight() const
    {
        return m_height;
    }

    /**
 * Invoked when the registered window instance is destroyed.
 */
    void OnWindowDestroy();

    /**
 * Destroy any resource that are used by the game.
 */
    void Destroy();

private:
    // Helper functions
    // Transition a resource
    void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    // Clear a render target view.
    void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

    // Clear the depth of a depth-stencil view.
    void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f );

    // Resize the depth buffer to match the size of the client area.
    void ResizeDepthBuffer(int width, int height);

    void _createSrvForFirstPassRTVs();

    void _createSamplers();
    
    uint64_t m_FenceValues[Window::BufferCount] = {};

    // Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
    // Descriptor heap for depth buffer as DSV
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

    // textures
    // index 0: imgui
	// index 1-3, 4-6, 7-9: first pass RTVs for each buffer
	// index 10: depth buffer SRV
	// index 11-13, 14-16, etc: other textures
    // total size: 512 (for now)
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplersHeap;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    bool m_ContentLoaded;

    Camera m_mainCamera;

    std::wstring m_name;
    int m_width;
    int m_height;
    bool m_isVSync;

    std::shared_ptr<Window> m_pWindow;
};