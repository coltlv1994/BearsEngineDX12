#pragma once

#include <Game.h>
#include <Window.h>
#include <Camera.h>

#include <DirectXMath.h>

#include <Shader.h>

#include <memory>
#include <utility>

class Editor : public Game
{
public:
    using super = Game;

    Editor(const std::wstring& name, int width, int height, bool vSync = true);
    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override;

    D3D12_VIEWPORT& GetViewport();

    D3D12_RECT& GetScissorRect();

	bool AddMesh(const std::wstring& meshPath, Shader* shader_p, const std::wstring& texturePath);

	// for debug purposes
	bool AddInstanceToMesh_DEBUG(const std::wstring& meshName);
protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override;

    /**
     *  Render stuff.
     */
    virtual void OnRender(RenderEventArgs& e) override;

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    virtual void OnKeyPressed(KeyEventArgs& e) override;

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;


    virtual void OnResize(ResizeEventArgs& e) override;

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

    unsigned char* ReadAndUploadTexture(const wchar_t* textureFilePath);
    
    uint64_t m_FenceValues[Window::BufferCount] = {};

    // Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
    // Descriptor heap for depth buffer.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;

    // Texture resources, temporarily we don't hold it in separate places
    // and only one texture is supported
    ComPtr<ID3D12Resource> m_texture = nullptr;
    ComPtr<ID3D12Resource> m_textureUploadHeap = nullptr;

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT m_ScissorRect;

    bool m_ContentLoaded;

    Camera m_mainCamera;
};