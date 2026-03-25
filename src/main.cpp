#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include <Application.h>
#include <Editor.h>
#include <Helpers.h>
#include <MeshManager.h>

#include <dxgidebug.h>

void ReportLiveObjects()
{
    //IDXGIDebug1* dxgiDebug;
    //DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    //dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    //dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#if defined(_DEBUG)
    // Load the WinPixGpuCapturer.dll for GPU capture in PIX. Adjust the path as needed.
    // ONLY WORKS IN DEBUG MODE.
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        LoadLibrary(L"C:\\Program Files\\Microsoft PIX\\2602.25\\WinPixGpuCapturer.dll");
    }
#endif

    int retCode = 0;

    Application::Create(hInstance);

    {
        std::shared_ptr<Editor> editor = std::make_shared<Editor>(L"BearsEngine in D3D12", 2560, 1440);

        Shader demoShader(L"FirstPassVertexShader", L"FirstPassPixelShader",
            L"SecondPassVertexShader", L"SecondPassPixelShader");

		Shader forwardShader(L"VertexShader", L"PixelShader");

		MeshManager::Get().SetDeferredRenderer(&demoShader);
        MeshManager::Get().SetForwardRenderer(&forwardShader);

        retCode = Application::Get().Run(editor);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}