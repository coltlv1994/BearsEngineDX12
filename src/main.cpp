#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shlwapi.h>

#include "Application.h"
#include "Helpers.h"
#include "MeshManager.h"

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
		LoadLibrary(L"C:\\Program Files\\Microsoft PIX\\2601.15\\WinPixGpuCapturer.dll");
	}
#endif

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

	retCode = Application::Get().RunWithBearWindow(L"BearWindow Editor", 1280, 720);

	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}