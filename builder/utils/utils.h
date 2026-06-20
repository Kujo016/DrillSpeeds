#pragma once
#include <cstdio>
#include <string>

#ifdef _WIN32
// -------------------- WINDOWS : DXGI --------------------
#include <windows.h>
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")
#endif

#ifdef _WIN32
inline double print_vram(int /*device*/ = 0)
{
    IDXGIFactory1* factory = nullptr;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (FAILED(hr) || !factory) {
        std::printf("[GPU] DXGI factory creation failed.\n");
        return -1.0;
    }

    double best_gb = -1.0;
    std::wstring best_name;

    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* adapter = nullptr;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;

        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
                double gb = double(desc.DedicatedVideoMemory) /
                            (1024.0 * 1024.0 * 1024.0);
                if (gb > best_gb) {
                    best_gb = gb;
                    best_name = desc.Description;
                }
            }
        }
        adapter->Release();
    }

    factory->Release();

    if (best_gb < 0.0) {
        std::printf("[GPU] No hardware GPU found.\n");
        return -1.0;
    }

    std::printf("[GPU] Adapter: %ls\n", best_name.c_str());
    std::printf("[GPU] VRAM: %.2f GB\n", best_gb);
    return best_gb;
}

#elif defined(__linux__)
// -------------------- LINUX : NO GRAPHICS BACKEND --------------------

inline double print_vram(int /*device*/ = 0)
{
    std::printf(
        "[GPU] VRAM query not available on Linux without a graphics backend.\n"
        "[GPU] (Vulkan, OpenCL, or vendor-specific DRM APIs are required.)\n"
    );
    return -1.0;
}

#else
// -------------------- OTHER PLATFORMS --------------------

inline double print_vram(int /*device*/ = 0)
{
    std::printf("[GPU] VRAM query not implemented on this platform.\n");
    return -1.0;
}
#endif

#ifdef _WIN32

inline bool gpu_present()
{
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory)) || !factory)
        return false;

    bool found = false;

    for (UINT i = 0;; ++i) {
        IDXGIAdapter1* adapter = nullptr;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0) {
                found = true;
                adapter->Release();
                break;
            }
        }
        adapter->Release();
    }

    factory->Release();
    return found;
}
#else
inline bool gpu_present() { return false; }
#endif


