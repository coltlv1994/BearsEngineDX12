#include <DX12LibPCH.h>

#include <Application.h>
#include <Game.h>
#include <Window.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

Game::Game( const std::wstring& name, int width, int height, bool vSync )
    : m_Name( name )
    , m_Width( width )
    , m_Height( height )
    , m_vSync( vSync )
{
}

Game::~Game()
{
    assert(!m_pWindow && "Use Game::Destroy() before destruction.");
}

bool Game::Initialize()
{
    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    m_pWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
    m_pWindow->RegisterCallbacks(shared_from_this());
    m_pWindow->Show();

    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(m_pWindow->GetWindowHandle());

    return true;
}

void Game::Destroy()
{
    Application::Get().DestroyWindow(m_pWindow);
    m_pWindow.reset();
}

void Game::OnUpdate(UpdateEventArgs& e)
{

}

void Game::OnRender(RenderEventArgs& e)
{

}

void Game::OnKeyPressed(KeyEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnKeyReleased(KeyEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseMoved(class MouseMotionEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseWheel(MouseWheelEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnResize(ResizeEventArgs& e)
{
    m_Width = e.Width;
    m_Height = e.Height;
}

void Game::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
}

