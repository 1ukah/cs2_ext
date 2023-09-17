#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <iostream>
#include <format>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

#include "hack.hpp"
#include "memory.hpp"
#include "offsets.hpp"
#include "utils.hpp"
#include "xorstr.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) 
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) 
        return 1L;

    switch (message)
    {
        case WM_DESTROY: 
        {
            PostQuitMessage(0);
            return 0L;
        }
    }

    return DefWindowProc(window, message, w_param, l_param);
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) 
{
#ifdef _DEBUG
    if (!AllocConsole())
        return FALSE;

    FILE* file{ nullptr };
    freopen_s(&file, XorStr("CONIN$"), XorStr("r"), stdin);
    freopen_s(&file, XorStr("CONOUT$"), XorStr("w"), stdout);
    freopen_s(&file, XorStr("CONOUT$"), XorStr("w"), stderr);

    //std::cout << XorStr("test") << std::endl;
#endif

    hack::process = std::make_shared<pProcess>();

    while (!hack::process->AttachProcess(XorStr("cs2.exe")))
        std::this_thread::sleep_for(std::chrono::seconds(1));

    do
    {
        hack::base_module = hack::process->GetModule(XorStr("client.dll"));
        if (hack::base_module.base == 0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << XorStr("failed to find module client.dll, waiting for the game to load it...") << std::endl;
        }
    } while (hack::base_module.base == 0);

    if (!FreeConsole())
        return FALSE;

    const WNDCLASSEXW wc
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = window_procedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = instance,
        .hIcon = nullptr,
        .hCursor = nullptr,
        .hbrBackground = nullptr,
        .lpszMenuName = nullptr,
        .lpszClassName = L" ",
        .hIconSm = nullptr
    };

    if (!RegisterClassExW(&wc))
        return FALSE;

    const HWND window = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L" ",
        WS_POPUP,
        0,
        0,
        1920,
        1080,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (!window)
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return FALSE;
    }

    if (!SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA))
    {
        DestroyWindow(window);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return FALSE;
    }

    {
        RECT client_area{};
        if (!GetClientRect(window, &client_area))
        {
            DestroyWindow(window);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return FALSE;
        }

        RECT window_area{};
        if (!GetWindowRect(window, &window_area))
        {
            DestroyWindow(window);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return FALSE;
        }

        POINT diff{};
        if (!ClientToScreen(window, &diff))
        {
            DestroyWindow(window);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return FALSE;
        }

        const MARGINS margins
        {
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        if (FAILED(DwmExtendFrameIntoClientArea(window, &margins)))
        {
            DestroyWindow(window);
            UnregisterClassW(wc.lpszClassName, wc.hInstance);
            return FALSE;
        }
    }

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));

    sd.BufferDesc.Width = 0U;
    sd.BufferDesc.Height = 0U;
    sd.BufferDesc.RefreshRate.Numerator = 240U;
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    sd.SampleDesc.Count = 1U;
    sd.SampleDesc.Quality = 0U;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1U;
    sd.OutputWindow = window;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    //DXGI_SWAP_EFFECT_DISCARD
    //DXGI_SWAP_EFFECT_FLIP_DISCARD
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    //DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH

    constexpr D3D_FEATURE_LEVEL feature_levels[2]{
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL feature_level{};

    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_context{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
        
    if (FAILED(D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0U,
        feature_levels,
        2U,
        D3D11_SDK_VERSION,
        &sd,
        &swap_chain,
        &device,
        &feature_level,
        &device_context))) {
        DestroyWindow(window);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return FALSE;
    }

    ID3D11Texture2D* back_buffer{ nullptr };

    if (FAILED(swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer))))
        return FALSE;

    if (FAILED(device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view)))
        return FALSE;

    if (back_buffer)
        back_buffer->Release();

    ShowWindow(window, cmd_show);
    UpdateWindow(window);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);

    bool running = true;
    while (running)
    {
        if (GetAsyncKeyState(VK_END) & 0x8000)
            running = false;

        if (GetAsyncKeyState(VK_F1) & 1)
            hack::menu = !hack::menu;

        if (GetAsyncKeyState(VK_F2) & 1)
            hack::watermark = !hack::watermark;

        if (GetAsyncKeyState(VK_F3) & 1)
            hack::crosshair = !hack::crosshair;

        if (GetAsyncKeyState(VK_F5) & 1)
            hack::showteam = !hack::showteam;

        if (GetAsyncKeyState(VK_F6) & 1)
            hack::bbox = !hack::bbox;

        if (GetAsyncKeyState(VK_F7) & 1)
            hack::pname = !hack::pname;

        if (GetAsyncKeyState(VK_F8) & 1)
            hack::phealth = !hack::phealth;

        if (GetAsyncKeyState(VK_F9) & 1)
            hack::snaplines = !hack::snaplines;

        if (GetAsyncKeyState(VK_F10) & 1)
            hack::skeletons = !hack::skeletons;

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
                running = false;
        }

        if (!running)
            break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (hack::watermark)
        {
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(8, 10), ImVec2(90, 24), ImColor(15, 15, 15, 140));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 10), ImColor(255, 0, 255, 255), XorStr("NIGLEY_HACK"));

            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(95, 10), ImVec2(185, 24), ImColor(15, 15, 15, 140));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(98, 10), ImColor(255, 0, 255, 255), XorStr("[END] Unload"));

            if (!hack::menu)
            {
                ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(8, 25), ImVec2(90, 39), ImColor(15, 15, 15, 140));
                ImGui::GetBackgroundDrawList()->AddText(ImVec2(10, 25), ImColor(255, 0, 255, 255), XorStr("[F1] Menu"));
            }
        }

        if (hack::menu)
        {
            ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(349, 24), ImVec2(500, 146), ImColor(15, 15, 15, 140));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 25), hack::watermark ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F2]  > Watermark"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 40), hack::crosshair ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F3]  > Crosshair"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 55), hack::showteam ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F5]  > Show Team"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 70), hack::bbox ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F6]  > Bounding Box"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 85), hack::pname ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F7]  > Player Name"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 100), hack::phealth ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F8]  > Player Health"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 115), hack::snaplines ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F9]  > Snaplines"));
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(350, 130), hack::skeletons ? ImColor(0, 255, 0, 255) : ImColor(255, 0, 0, 255), XorStr("[F10] > Skeletons"));
        }

        if (hack::crosshair)
        {
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(1920 / 2 - 5, 1080 / 2), ImVec2(1920 / 2 + 6, 1080 / 2), ImColor(255, 255, 255, 255));
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2(1920 / 2, 1080 / 2 - 5), ImVec2(1920 / 2, 1080 / 2 + 6), ImColor(255, 255, 255, 255));
        }

        hack::loop();

        ImGui::Render();

        constexpr float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_context->ClearRenderTargetView(render_target_view, clear_color);
        
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swap_chain->Present(0U, 0U);

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (swap_chain)
    {
        swap_chain->Release();
        swap_chain = nullptr;
    }

    if (device_context)
    {
        device_context->Release();
        device_context = nullptr;
    }

    if (device)
    {
        device->Release();
        device = nullptr;
    }

    if (render_target_view)
    {
        render_target_view->Release();
        render_target_view = nullptr;
    }

    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return TRUE;
}
