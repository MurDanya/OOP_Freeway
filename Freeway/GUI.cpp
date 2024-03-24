#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "Model.h"
#include <tchar.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <d3d9.h>
#include <D3dx9tex.h>
#pragma comment(lib, "D3dx9")


// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


// Загрузка изображений
bool LoadTextureFromFile(const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
    PDIRECT3DTEXTURE9 texture;
    HRESULT hr = D3DXCreateTextureFromFileA(g_pd3dDevice, filename, &texture);
    if (hr != S_OK)
        return false;

    D3DSURFACE_DESC my_image_desc;
    texture->GetLevelDesc(0, &my_image_desc);
    *out_texture = texture;
    *out_width = (int)my_image_desc.Width;
    *out_height = (int)my_image_desc.Height;
    return true;
}



// Параметры дороги
float scale_x, scale_y;
int image_road_width = 0, image_road_height = 0;
int car_width = 0, car_height = 0;
PDIRECT3DTEXTURE9 image_road = NULL, green_car = NULL, yellow_car = NULL, red_car = NULL, orange_car = NULL, image_car = NULL,
    choose_green_car = NULL, choose_yellow_car = NULL, choose_red_car = NULL, choose_orange_car = NULL;
std::vector<int> parts_road = {1073, 1387, 2400, 2714, 3727, 4041, 5054, 5368, 6434};
std::vector<std::pair<int, int>> begins_parts_road = { {643, 130}, {1716, 230}, {1716, 330}, {703, 430}, {703, 530}, {1716, 630}, {1716, 730}, {703, 830}, {703, 930} };
int hx = 60, hy = 30;
int true_road_width = 1920, true_road_height = 1080;
int r = 100;


// Подсчет размещения машинок
PDIRECT3DTEXTURE9 coord_image_car(const Auto* car, ImVec2& p1, ImVec2& p2, ImVec2& p3, ImVec2& p4) {
    double coord = car->get_coord() / LEN_ROAD * parts_road.back();
    int num_part = 0;
    while (num_part < parts_road.size() && coord > parts_road[num_part]) {
        ++num_part;
    }
    int x = begins_parts_road[num_part].first;
    int y = begins_parts_road[num_part].second;
    int dx = (num_part ? coord - parts_road[num_part - 1] : coord);
    bool k = (num_part % 4) < 2;
    if (num_part % 2 == 0) {
        if (k) {
            x += dx;
            p1 = { (x - hx) * scale_x, (y + hy) * scale_y };
            p2 = { (x + hx) * scale_x, (y + hy) * scale_y };
            p3 = { (x + hx) * scale_x, (y - hy) * scale_y };
            p4 = { (x - hx) * scale_x, (y - hy) * scale_y };
        }
        else {
            x -= dx;
            p1 = { (x + hx) * scale_x, (y - hy) * scale_y };
            p2 = { (x - hx) * scale_x, (y - hy) * scale_y };
            p3 = { (x - hx) * scale_x, (y + hy) * scale_y };
            p4 = { (x + hx) * scale_x, (y + hy) * scale_y };
        }
    }
    else {
        float cos_a = cos((float)dx / r);
        float sin_a = sin((float)dx / r);
        if (k) {
            p1 = { (x + (r + hy) * sin_a - hx * cos_a) * scale_x, (y - (r + hy) * cos_a - hx * sin_a) * scale_y };
            p2 = { (x + (r + hy) * sin_a + hx * cos_a) * scale_x, (y - (r + hy) * cos_a + hx * sin_a) * scale_y };
            p3 = { (x + (r - hy) * sin_a + hx * cos_a) * scale_x, (y - (r - hy) * cos_a + hx * sin_a) * scale_y };
            p4 = { (x + (r - hy) * sin_a - hx * cos_a) * scale_x, (y - (r - hy) * cos_a - hx * sin_a) * scale_y };
        }
        else {
            sin_a *= -1;
            p3 = { (x + (r + hy) * sin_a - hx * cos_a) * scale_x, (y - (r + hy) * cos_a - hx * sin_a) * scale_y };
            p4 = { (x + (r + hy) * sin_a + hx * cos_a) * scale_x, (y - (r + hy) * cos_a + hx * sin_a) * scale_y };
            p1 = { (x + (r - hy) * sin_a + hx * cos_a) * scale_x, (y - (r - hy) * cos_a + hx * sin_a) * scale_y };
            p2 = { (x + (r - hy) * sin_a - hx * cos_a) * scale_x, (y - (r - hy) * cos_a - hx * sin_a) * scale_y };
        }
        
    }
    if (car->get_status() == CONSTANT_SPEED) {
        return green_car;
    }
    else if (car->get_status() == ACCELERATION) {
        return yellow_car;
    }
    else if (car->get_status() == SLOWDOWN) {
        return orange_car;
    }
    return red_car;
}


PDIRECT3DTEXTURE9 coord_choosen_car(const Auto* car, ImVec2& p1, ImVec2& p2, ImVec2& p3, ImVec2& p4) {
    double coord = car->get_coord() / LEN_ROAD * parts_road.back();
    int num_part = 0;
    while (num_part < parts_road.size() && coord > parts_road[num_part]) {
        ++num_part;
    }
    int x = begins_parts_road[num_part].first;
    int y = begins_parts_road[num_part].second;
    int dx = (num_part ? coord - parts_road[num_part - 1] : coord);
    bool k = (num_part % 4) < 2;
    int d = 10;
    hx += d;
    hy += d;
    if (num_part % 2 == 0) {
        if (k) {
            x += dx;
            p1 = { (x - hx) * scale_x, (y + hy) * scale_y };
            p2 = { (x + hx) * scale_x, (y + hy) * scale_y };
            p3 = { (x + hx) * scale_x, (y - hy) * scale_y };
            p4 = { (x - hx) * scale_x, (y - hy) * scale_y };
        }
        else {
            x -= dx;
            p1 = { (x + hx) * scale_x, (y - hy) * scale_y };
            p2 = { (x - hx) * scale_x, (y - hy) * scale_y };
            p3 = { (x - hx) * scale_x, (y + hy) * scale_y };
            p4 = { (x + hx) * scale_x, (y + hy) * scale_y };
        }
    }
    else {
        float cos_a = cos((float)dx / r);
        float sin_a = sin((float)dx / r);
        if (k) {
            p1 = { (x + (r + hy) * sin_a - hx * cos_a) * scale_x, (y - (r + hy) * cos_a - hx * sin_a) * scale_y };
            p2 = { (x + (r + hy) * sin_a + hx * cos_a) * scale_x, (y - (r + hy) * cos_a + hx * sin_a) * scale_y };
            p3 = { (x + (r - hy) * sin_a + hx * cos_a) * scale_x, (y - (r - hy) * cos_a + hx * sin_a) * scale_y };
            p4 = { (x + (r - hy) * sin_a - hx * cos_a) * scale_x, (y - (r - hy) * cos_a - hx * sin_a) * scale_y };
        }
        else {
            sin_a *= -1;
            p3 = { (x + (r + hy) * sin_a - hx * cos_a) * scale_x, (y - (r + hy) * cos_a - hx * sin_a) * scale_y };
            p4 = { (x + (r + hy) * sin_a + hx * cos_a) * scale_x, (y - (r + hy) * cos_a + hx * sin_a) * scale_y };
            p1 = { (x + (r - hy) * sin_a + hx * cos_a) * scale_x, (y - (r - hy) * cos_a + hx * sin_a) * scale_y };
            p2 = { (x + (r - hy) * sin_a - hx * cos_a) * scale_x, (y - (r - hy) * cos_a - hx * sin_a) * scale_y };
        }

    }
    hx -= d;
    hy -= d;
    if (car->get_status() == CONSTANT_SPEED) {
        return choose_green_car;
    }
    else if (car->get_status() == ACCELERATION) {
        return choose_yellow_car;
    }
    else if (car->get_status() == SLOWDOWN) {
        return choose_orange_car;
    }
    return choose_red_car;
}


bool
in_rectangle(float x, float y, ImVec2& p1, ImVec2& p2, ImVec2& p3, ImVec2& p4) {
    int k1, k2, k3, k4;
    k1 = (x - p1.x) * (p2.y - p1.y) - (y - p1.y) * (p2.x - p1.x);
    k2 = (x - p2.x) * (p3.y - p2.y) - (y - p2.y) * (p3.x - p2.x);
    k3 = (x - p3.x) * (p4.y - p3.y) - (y - p3.y) * (p4.x - p3.x);
    k4 = (x - p4.x) * (p1.y - p4.y) - (y - p4.y) * (p1.x - p4.x);
    return (k1 > 0 && k2 > 0 && k3 > 0 && k4 > 0) || (k1 < 0 && k2 < 0 && k3 < 0 && k4 < 0);
}



// Main code
int freewayWithGui()
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Freeway", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    //auto prev_frame = std::chrono::high_resolution_clock::now();
    //auto cur_prev = std::chrono::high_resolution_clock::now();

    // --------------------------------------------------------------------------------------------------------------------------------------------------------


    bool ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\road.png", &image_road, &image_road_width, &image_road_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\green_car.png", &green_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\yellow_car.png", &yellow_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\orange_car.png", &orange_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\red_car.png", &red_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\choosed_green_car.png", &choose_green_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\choosed_yellow_car.png", &choose_yellow_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\choosed_orange_car.png", &choose_orange_car, &car_width, &car_height);
    IM_ASSERT(ret);
    ret = LoadTextureFromFile("C:\\Users\\qwert\\Desktop\\progs\\prak\\OOP\\Freeway\\images\\choosed_red_car.png", &choose_red_car, &car_width, &car_height);
    IM_ASSERT(ret);


    // Параметры GUI
    Model *model = new Model();                                                             // Модель
    ImVec2 p1, p2, p3, p4;                                                                  // Координаты для машин
    int min_speed = CONST_MIN_SPEED, max_speed = CONST_MAX_SPEED;
    int min_time = CONST_MIN_TIME, max_time = CONST_MAX_TIME;
    float coef_acc = 1, coef_slow = 1;                                                      // Параметры модели
    char val_str[64];
    float log_value;
    float min_coef = log(0.1), max_coef = log(10);                                          // Дополнительные переменные для логарифмического слайдера
    bool live = false;                                                                      // Непрерывная работа
    bool is_begin = true;                                                                   // начало работы
    bool artificial_delay = false;                                                          // Искусственное замедление
    int choosed_car = -1, choosed_speed = 0, choosed_time = CONST_MIN_TIME;
    Auto *choose_auto = NULL;
    int choosed_cur_speed = 0;                                                              // Парметры искусственного замедления


    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        


        scale_x = ImGui::GetIO().DisplaySize.x / true_road_width;
        scale_y = ImGui::GetIO().DisplaySize.y / true_road_height;
        ImGui::GetBackgroundDrawList()->AddImage((void*)image_road, ImVec2(0, 0), ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y));
        
        int i = 0;
        int x = ImGui::GetIO().MousePos.x;
        int y = ImGui::GetIO().MousePos.y;
        for (auto& car : model->get_road().get_list_auto()) {
            if (choosed_car == i) {
                image_car = coord_choosen_car(&car, p1, p2, p3, p4);
            }
            else {
                image_car = coord_image_car(&car, p1, p2, p3, p4);
            }
            if (artificial_delay && ImGui::IsMousePosValid() && ImGui::IsMouseDown(0)) {
                if (in_rectangle(x, y, p1, p2, p3, p4)) {
                    choosed_car = i;
                    choosed_cur_speed = (int)car.get_speed();
                    choose_auto = &car;
                    choosed_speed = 0;
                    choosed_time = CONST_MIN_TIME;
                }
            }
            ImGui::GetBackgroundDrawList()->AddImageQuad((void*)image_car, p1, p2, p3, p4);
            ++i;
        }

        if (live) {
            model->set_params(min_time, max_time, min_speed, max_speed, coef_acc, coef_slow);
            model->tick();
        }

        {
            ImGuiWindowFlags window_flags = 0;
            /*
            window_flags |= ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoResize;
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
            */

            ImGui::Begin("Parametrs", NULL, window_flags);

            ImGui::Text("%d FPS", (int)io.Framerate);
            ImGui::NewLine();

            ImGui::Text("Min Init Speed");
            ImGui::SliderInt(" ", &min_speed, CONST_MIN_SPEED, max_speed, "%d kmh");
            ImGui::Text("Max Init Speed");
            ImGui::SliderInt("  ", &max_speed, min_speed, CONST_MAX_SPEED, "%d kmh");
            ImGui::NewLine();

            ImGui::Text("Min time of appear");
            ImGui::SliderInt("   ", &min_time, CONST_MIN_TIME, max_time, "%d sec");
            ImGui::Text("Max time of appear");
            ImGui::SliderInt("    ", &max_time, min_time, CONST_MAX_TIME, "%d sec");
            ImGui::NewLine();

            ImGui::Text("Acceleration coefficient");
            sprintf_s(val_str, "%.2f", coef_acc);
            log_value = log(coef_acc);
            ImGui::SliderFloat("     ", &log_value , min_coef, max_coef, val_str);
            coef_acc = exp(log_value);
            ImGui::Text("Deceleration coefficient");
            sprintf_s(val_str, "%.2f", coef_slow);
            log_value = log(coef_slow);
            ImGui::SliderFloat("      ", &log_value, min_coef, max_coef, val_str);
            coef_slow = exp(log_value);
            ImGui::NewLine();

            if (is_begin) {
                if (ImGui::Button("Start", ImVec2(100, 50))) {
                    is_begin = false;
                    live = true;
                }
            }
            else {
                if (live) {
                    if (ImGui::Button("Pause", ImVec2(100, 50))) {
                        live = false;
                    }
                }
                else {
                    if (ImGui::Button("Continue", ImVec2(100, 50))) {
                        live = true;
                        artificial_delay = false;
                        choosed_car = -1;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Restart", ImVec2(100 - ImGui::GetStyle().ItemSpacing.x, 50))) {
                    delete model;
                    model = new Model();
                    is_begin = true;
                    live = false;
                    artificial_delay = false;
                    choosed_car = -1;
                }
                if (!artificial_delay) {
                    if (ImGui::Button("Artificial delay", ImVec2(200, 50))) {
                        live = false;
                        artificial_delay = true;
                        choosed_car = -1;
                        choosed_speed = 0;
                        choosed_time = CONST_MIN_TIME;
                    }
                }
            }

            ImGui::End();

            if (artificial_delay) {
                ImGui::Begin("Artificial delay", NULL, window_flags);
                if (choosed_car == -1) {
                    ImGui::Text("Choose a car for artificial delay");
                }
                else {
                    ImGui::Text("Set the speed and delay time");
                    ImGui::NewLine();
                    ImGui::Text("Speed");
                    ImGui::SliderInt("        ", &choosed_speed, 0, choosed_cur_speed, "%d kmh");
                    ImGui::Text("Time");
                    ImGui::SliderInt("         ", &choosed_time, CONST_MIN_TIME, CONST_MAX_TIME, "%d sec");
                    if (ImGui::Button("Ok")) {
                        choose_auto->artificial_delay(choosed_speed, choosed_time);
                        choosed_car = -1;
                        choosed_speed = 0;
                        choosed_time = CONST_MIN_TIME;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        choosed_car = -1;
                        choosed_speed = 0;
                        choosed_time = CONST_MIN_TIME;
                    }
                }
                ImGui::End();
            }
        }


        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    delete model;

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
