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
    int retCode = 0;

    // Set the working directory to the path of the executable.
    //WCHAR path[MAX_PATH];
    //HMODULE hModule = GetModuleHandleW(NULL);
    //if (GetModuleFileNameW(hModule, path, MAX_PATH) > 0)
    //{
    //    PathRemoveFileSpecW(path);
    //    SetCurrentDirectoryW(path);
    //}

    Application::Create(hInstance);

    {
        std::shared_ptr<Editor> editor = std::make_shared<Editor>(L"BearsEngine in D3D12", 2560, 1440);

        Shader demoShader(L"FirstPassVertexShader", L"FirstPassPixelShader",
            L"SecondPassVertexShader", L"SecondPassPixelShader");

		MeshManager::Get().SetDefaultShader(&demoShader);

        retCode = Application::Get().Run(editor);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}