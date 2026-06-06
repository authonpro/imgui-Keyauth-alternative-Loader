// ============================================================
//  AUTHON.PRO LOADER - Premium UI
//  DirectX 11 + ImGui
//  Page 1: Login (Beryllium-style)
//  https://authon.pro
// ============================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "authon.h"
#include "config.h"
#include "image_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include "imgui/stb_image.h"

#pragma comment(lib, "d3d11.lib")

// ============ STATE ============
enum class Page { LOGIN, LOADING, DASHBOARD };
enum class Tab { NEWS, FILES, ACCOUNT };
Page g_page = Page::LOGIN;
Tab g_tab = Tab::FILES;

char g_user[64] = {};
char g_pass[64] = {};
char g_key[128] = {};
char g_status[256] = {};
bool g_rememberMe = false;
bool g_signUpMode = false;
bool g_loginInProgress = false; // Prevent double-click
float g_dashAnim = 0.f; // Dashboard page entrance animation
bool g_dashAnimating = false;
float g_slideAnim = 1.f; // Start at 1 = fully visible (no animation on launch)
bool g_animating = false;
bool g_pendingMode = false;

// Toast notification system
char g_toast[256] = {};
float g_toastTimer = 0.f;
bool g_toastVisible = false;
bool g_toastIsError = true;

void ShowToast(const char* msg, bool isError = true) {
    strcpy_s(g_toast, msg);
    g_toastTimer = 3.5f; // 3.5 seconds visible
    g_toastVisible = true;
    g_toastIsError = isError;
}

void DrawToast(const ImGuiIO& io) {
    if (!g_toastVisible) return;

    g_toastTimer -= io.DeltaTime;
    if (g_toastTimer <= 0.f) { g_toastVisible = false; return; }

    // Fade in/out
    float alpha = 1.f;
    if (g_toastTimer > 3.f) alpha = 1.f - (g_toastTimer - 3.f) / 0.5f; // fade in
    if (g_toastTimer < 0.5f) alpha = g_toastTimer / 0.5f; // fade out
    if (alpha < 0.f) alpha = 0.f;
    if (alpha > 1.f) alpha = 1.f;

    // Slide from right
    float slideX = 0.f;
    if (g_toastTimer > 3.f) slideX = (g_toastTimer - 3.f) / 0.5f * 200.f;
    if (g_toastTimer < 0.5f) slideX = (0.5f - g_toastTimer) / 0.5f * 200.f;

    float toastW = 280.f;
    float toastH = 50.f;
    float toastX = io.DisplaySize.x - toastW - 15.f + slideX;
    float toastY = io.DisplaySize.y - toastH - 15.f;

    auto* dl = ImGui::GetForegroundDrawList();
    ImU32 bgCol = g_toastIsError ? IM_COL32(180, 40, 40, (int)(200 * alpha)) : IM_COL32(40, 160, 80, (int)(200 * alpha));
    ImU32 borderCol = g_toastIsError ? IM_COL32(220, 60, 60, (int)(150 * alpha)) : IM_COL32(60, 200, 100, (int)(150 * alpha));
    ImU32 textCol = IM_COL32(255, 255, 255, (int)(240 * alpha));

    dl->AddRectFilled(ImVec2(toastX, toastY), ImVec2(toastX + toastW, toastY + toastH), bgCol, 8.f);
    dl->AddRect(ImVec2(toastX, toastY), ImVec2(toastX + toastW, toastY + toastH), borderCol, 8.f);
    dl->AddText(ImVec2(toastX + 15, toastY + 16), textCol, g_toast);
}

authon::Authon* g_auth = nullptr;
ID3D11ShaderResourceView* g_bannerTexture = nullptr;
int g_bannerW = 0, g_bannerH = 0;

// ============ HWID ============
std::string GetHWID() {
    DWORD serial = 0;
    GetVolumeInformationA("C:\\", NULL, 0, &serial, NULL, NULL, NULL, 0);
    char buf[32]; sprintf_s(buf, "HWID-%08X", serial);
    return std::string(buf);
}

// ============ AUTH ============
void DoLogin() {
    if (g_loginInProgress) return; // Prevent double-click
    if (strlen(g_user) == 0 || strlen(g_pass) == 0) { ShowToast("Username and password required."); return; }
    if (g_signUpMode && strlen(g_key) == 0) { ShowToast("License key required."); return; }

    g_loginInProgress = true;
    ShowToast("Logging in...", false);

    std::thread([] {
        std::string hwid = GetHWID();
        bool ok = false;

        if (g_signUpMode) {
            ok = g_auth->registerUser(std::string(g_user), std::string(g_pass), std::string(g_key), hwid);
            if (ok) {
                ShowToast("Account created! Please sign in.", false);
                g_signUpMode = false;
                g_animating = true; g_slideAnim = 0.f; g_pendingMode = false;
                g_loginInProgress = false;
                return;
            } else {
                ShowToast("Registration failed. Check credentials.");
                g_loginInProgress = false;
                return;
            }
        } else {
            ok = g_auth->login(std::string(g_user), std::string(g_pass), hwid);
        }

        if (!ok) {
            ShowToast("Invalid username or password.");
            g_loginInProgress = false;
            return;
        }
        ShowToast("Login successful!", false);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        g_dashAnim = 0.f;
        g_dashAnimating = true;
        g_page = Page::DASHBOARD;
        g_loginInProgress = false;
    }).detach();
}

// ============ THEME ============
void SetTheme() {
    auto& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.FrameRounding = 4.f;
    s.ChildRounding = 6.f;
    s.GrabRounding = 4.f;
    s.PopupRounding = 6.f;
    s.ScrollbarRounding = 4.f;
    s.FramePadding = ImVec2(10, 8);
    s.ItemSpacing = ImVec2(6, 6);
    s.WindowPadding = ImVec2(0, 0);
    s.FrameBorderSize = 1.f;
    s.WindowBorderSize = 0.f;
    s.ChildBorderSize = 0.f;

    auto* c = s.Colors;
    // Dark background
    c[ImGuiCol_WindowBg]       = ImVec4(0.09f, 0.09f, 0.11f, 1.f);
    c[ImGuiCol_ChildBg]        = ImVec4(0.11f, 0.11f, 0.13f, 1.f);
    c[ImGuiCol_Border]         = ImVec4(0.25f, 0.25f, 0.30f, 0.4f);
    // Input fields
    c[ImGuiCol_FrameBg]        = ImVec4(0.13f, 0.13f, 0.16f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.20f, 1.f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.18f, 0.18f, 0.22f, 1.f);
    // Purple buttons (Authon brand)
    c[ImGuiCol_Button]         = ImVec4(0.486f, 0.228f, 0.929f, 1.f);  // #7c3aed
    c[ImGuiCol_ButtonHovered]  = ImVec4(0.55f, 0.30f, 0.96f, 1.f);
    c[ImGuiCol_ButtonActive]   = ImVec4(0.42f, 0.18f, 0.85f, 1.f);
    // Text
    c[ImGuiCol_Text]           = ImVec4(0.93f, 0.93f, 0.95f, 1.f);
    c[ImGuiCol_TextDisabled]   = ImVec4(0.45f, 0.45f, 0.50f, 1.f);
    // Check
    c[ImGuiCol_CheckMark]      = ImVec4(0.486f, 0.228f, 0.929f, 1.f);
    c[ImGuiCol_Separator]      = ImVec4(0.2f, 0.2f, 0.25f, 0.5f);
}

// ============ RENDER ============
void RenderUI() {
    const ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    // ======== LOGIN PAGE ========
    if (g_page == Page::LOGIN) {
        // Split: Left = login form, Right = info panel
        float leftW = io.DisplaySize.x * 0.52f;
        float rightW = io.DisplaySize.x - leftW;

        // --- LEFT PANEL (Login Form) ---
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.09f, 0.11f, 1.f));
        ImGui::BeginChild("##left", ImVec2(leftW, io.DisplaySize.y), false);

        // Staggered animation - each element drops from top with delay
        if (g_animating) {
            g_slideAnim += io.DeltaTime * 4.f;
            if (g_slideAnim >= 1.f) {
                g_slideAnim = 1.f;
                g_animating = false;
            }
            if (g_slideAnim > 0.1f && g_signUpMode != g_pendingMode) {
                g_signUpMode = g_pendingMode;
            }
        }

        // Helper: staggered alpha+offset for each element (index 0-7)
        auto getItemAlpha = [&](int index) -> float {
            if (!g_animating && g_slideAnim >= 1.f) return 1.f;
            float delay = index * 0.08f; // 80ms stagger between items
            float progress = (g_slideAnim - delay) / 0.4f;
            if (progress < 0.f) return 0.f;
            if (progress > 1.f) return 1.f;
            return progress;
        };
        auto getItemOffsetY = [&](int index) -> float {
            float alpha = getItemAlpha(index);
            return (1.f - alpha) * -15.f; // drops 15px from above
        };

        float pad = 40.f;
        float contentW = leftW - 80.f;
        float startY = 20.f; // Fixed top position instead of centering

        // AUTHON.PRO title - CENTERED
        ImGui::SetCursorPos(ImVec2(0, startY + getItemOffsetY(0)));
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(0));
        ImGui::PushFont(io.Fonts->Fonts[1]);
        float titleW = ImGui::CalcTextSize("AUTHON.PRO").x;
        ImGui::SetCursorPosX((leftW - titleW) * 0.5f);
        ImGui::TextColored(ImVec4(0.486f, 0.228f, 0.929f, 1.f), "AUTHON");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(0.655f, 0.545f, 0.980f, 1.f), ".PRO");
        ImGui::PopFont();

        // Subtitle centered
        float subW = ImGui::CalcTextSize("PROTECT YOUR SOFTWARE").x;
        ImGui::SetCursorPosX((leftW - subW) * 0.5f);
        ImGui::TextColored(ImVec4(0.486f, 0.228f, 0.929f, 0.7f), "PROTECT YOUR SOFTWARE");
        ImGui::PopStyleVar();

        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();

        // AUTHORIZATION label centered
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(1));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + getItemOffsetY(1));
        ImGui::PushFont(io.Fonts->Fonts[2]);
        float authW = ImGui::CalcTextSize("AUTHORIZATION").x;
        ImGui::SetCursorPosX((leftW - authW) * 0.5f);
        ImGui::Text("AUTHORIZATION");
        ImGui::PopFont();
        ImGui::PopStyleVar();

        ImGui::Spacing(); ImGui::Spacing();

        // Username input - with icon
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(2));
        ImGui::SetCursorPosX(pad);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + getItemOffsetY(2));
        ImGui::PushItemWidth(contentW);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(32, 9));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);

        ImVec2 inputPos = ImGui::GetCursorScreenPos();
        ImGui::InputTextWithHint("##user", "Username", g_user, sizeof(g_user));
        // Draw person icon
        auto* dlLeft = ImGui::GetWindowDrawList();
        ImGui::PushFont(io.Fonts->Fonts[3]);
        dlLeft->AddText(ImVec2(inputPos.x + 10, inputPos.y + 7), IM_COL32(130, 90, 220, 200), "\xee\x81\xb7"); // E077 person
        ImGui::PopFont();

        ImGui::Spacing();

        // Password input with lock icon
        ImGui::SetCursorPosX(pad);
        ImVec2 passPos = ImGui::GetCursorScreenPos();
        ImGui::InputTextWithHint("##pass", "Password", g_pass, sizeof(g_pass), ImGuiInputTextFlags_Password);
        ImGui::PushFont(io.Fonts->Fonts[3]);
        dlLeft->AddText(ImVec2(passPos.x + 10, passPos.y + 7), IM_COL32(130, 90, 220, 200), "\xee\x81\x8a"); // E04A lock
        ImGui::PopFont();

        ImGui::PopStyleVar(2);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(); // Alpha for inputs

        // License Key input
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(3));
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + getItemOffsetY(3));
        ImGui::Spacing();
        ImGui::SetCursorPosX(pad);
        ImGui::PushItemWidth(contentW);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(32, 9));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        if (g_signUpMode) {
            ImVec2 keyPos = ImGui::GetCursorScreenPos();
            ImGui::InputTextWithHint("##key", "License Key", g_key, sizeof(g_key));
            ImGui::PushFont(io.Fonts->Fonts[3]);
            dlLeft->AddText(ImVec2(keyPos.x + 10, keyPos.y + 7), IM_COL32(130, 90, 220, 200), "\xee\x82\x92");
            ImGui::PopFont();
        } else {
            // Invisible placeholder - exact same size as the input above
            static char dummy[2] = {};
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);
            ImGui::InputText("##keyplaceholder", dummy, sizeof(dummy), ImGuiInputTextFlags_ReadOnly);
            ImGui::PopStyleVar();
        }
        ImGui::PopStyleVar(2);
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(); // Alpha for license key

        ImGui::Spacing();

        // Remember Me checkbox - left aligned, smaller frame
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(4));
        ImGui::SetCursorPosX(pad);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 3));
        ImGui::Checkbox("Remember Me", &g_rememberMe);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(); // Alpha

        ImGui::Spacing(); ImGui::Spacing();

        // SIGN IN / SIGN UP button centered
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(5));
        ImGui::SetCursorPosX(pad);
        if (ImGui::Button(g_signUpMode ? "SIGN UP" : "SIGN IN", ImVec2(contentW, 38))) DoLogin();
        ImGui::PopStyleVar();

        ImGui::Spacing();

        // Toggle sign in / sign up - inline text button
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, getItemAlpha(6));
        if (!g_signUpMode) {
            float dontW2 = ImGui::CalcTextSize("Don't have an account?  Sign Up").x;
            ImGui::SetCursorPosX((leftW - dontW2) * 0.5f);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.f), "Don't have an account?");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.486f, 0.228f, 0.929f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            if (ImGui::Button("Sign Up##toggle")) { g_animating = true; g_slideAnim = 0.f; g_pendingMode = true; }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(5);
        } else {
            float alreadyW = ImGui::CalcTextSize("Already have an account?  Sign In").x;
            ImGui::SetCursorPosX((leftW - alreadyW) * 0.5f);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.f), "Already have an account?");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.486f, 0.228f, 0.929f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            if (ImGui::Button("Sign In##toggle")) { g_animating = true; g_slideAnim = 0.f; g_pendingMode = false; }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(5);
        }
        ImGui::PopStyleVar(); // Alpha for toggle

        ImGui::Spacing();

        // Footer - fixed at bottom
        float footW = ImGui::CalcTextSize("Authon @ 2026").x;
        ImGui::SetCursorPos(ImVec2((leftW - footW) * 0.5f, io.DisplaySize.y - 25));
        ImGui::TextColored(ImVec4(0.3f, 0.3f, 0.35f, 1.f), "Authon @ 2026");

        ImGui::EndChild();
        ImGui::PopStyleColor();

        // --- RIGHT PANEL - Image fills entire panel, text overlaid on top ---
        ImGui::SetCursorPos(ImVec2(leftW, 0));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.06f, 1.f));
        ImGui::BeginChild("##right", ImVec2(rightW, io.DisplaySize.y), false);

        // Background image - fill entire right panel
        if (g_bannerTexture) {
            ImGui::SetCursorPos(ImVec2(0, 0));
            ImGui::Image((ImTextureID)g_bannerTexture, ImVec2(rightW, io.DisplaySize.y));
        }

        // Overlay: slight dark on top for — X visibility
        auto* dlRight = ImGui::GetWindowDrawList();
        ImVec2 panelPos = ImGui::GetWindowPos();
        dlRight->AddRectFilledMultiColor(
            ImVec2(panelPos.x, panelPos.y),
            ImVec2(panelPos.x + rightW, panelPos.y + 45),
            IM_COL32(10, 10, 15, 160), IM_COL32(10, 10, 15, 160),
            IM_COL32(10, 10, 15, 0), IM_COL32(10, 10, 15, 0)
        );

        // — X buttons
        ImGui::SetCursorPos(ImVec2(rightW - 60, 10));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.45f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button(" - ##min", ImVec2(24, 24))) {
            ShowWindow(FindWindowW(L"AuthonLoaderWnd", NULL), SW_MINIMIZE);
        }
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.15f, 0.15f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 0.7f));
        if (ImGui::Button(" X ##close", ImVec2(24, 24))) {
            ExitProcess(0);
        }
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);

        ImGui::EndChild();
        ImGui::PopStyleColor();

    // ======== LOADING PAGE ========
    } else if (g_page == Page::LOADING) {
        float cx = io.DisplaySize.x * 0.5f;
        float cy = io.DisplaySize.y * 0.5f;
        ImGui::SetCursorPos(ImVec2(cx - 80, cy - 10));
        ImGui::Text("%s", g_status);

    // ======== DASHBOARD ========
    } else if (g_page == Page::DASHBOARD) {
        // Dashboard entrance animation
        if (g_dashAnimating) {
            g_dashAnim += io.DeltaTime * 3.5f;
            if (g_dashAnim >= 1.f) { g_dashAnim = 1.f; g_dashAnimating = false; }
        }
        auto dashAlpha = [&](int idx) -> float {
            if (!g_dashAnimating && g_dashAnim >= 1.f) return 1.f;
            float delay = idx * 0.1f;
            float progress = (g_dashAnim - delay) / 0.35f;
            if (progress < 0.f) return 0.f;
            if (progress > 1.f) return 1.f;
            return progress;
        };
        auto dashOffsetY = [&](int idx) -> float {
            return (1.f - dashAlpha(idx)) * -20.f;
        };

        // ---- Top bar (purple accent line + title + X) ----
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dashAlpha(0));

        // Purple accent line at very top
        auto* dlDash = ImGui::GetWindowDrawList();
        ImVec2 winPos = ImGui::GetWindowPos();
        dlDash->AddRectFilled(
            ImVec2(winPos.x, winPos.y),
            ImVec2(winPos.x + io.DisplaySize.x, winPos.y + 3),
            IM_COL32(124, 58, 237, 255) // Purple accent line
        );

        // Top bar background
        dlDash->AddRectFilled(
            ImVec2(winPos.x, winPos.y + 3),
            ImVec2(winPos.x + io.DisplaySize.x, winPos.y + 40),
            IM_COL32(18, 18, 24, 255)
        );

        // Title in top bar - left aligned, bigger font
        ImGui::SetCursorPos(ImVec2(15, 8 + dashOffsetY(0)));
        ImGui::PushFont(io.Fonts->Fonts[1]); // Use title font (bigger)
        ImGui::TextColored(ImVec4(0.486f, 0.228f, 0.929f, 1.f), "AUTHON");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(ImVec4(0.655f, 0.545f, 0.980f, 1.f), ".PRO");
        ImGui::PopFont();

        // - and X buttons in top bar
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - 60, 8));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.4f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        if (ImGui::Button(" - ##dashmin", ImVec2(24, 24))) {
            HWND hw = FindWindowW(L"AuthonLoaderWnd", NULL);
            if (hw) ShowWindow(hw, SW_MINIMIZE);
        }
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.15f, 0.15f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.1f, 0.1f, 0.7f));
        if (ImGui::Button(" X ##dashclose", ImVec2(24, 24))) {
            ExitProcess(0);
        }
        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(2);

        ImGui::PopStyleVar();

        // ---- Content below top bar ----
        // Left sidebar with icon buttons
        float sidebarW = 55.f;
        float sidebarTop = 45.f;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dashAlpha(1));

        // Sidebar background - starts BELOW top bar (40px)
        dlDash->AddRectFilled(
            ImVec2(winPos.x, winPos.y + 40),
            ImVec2(winPos.x + sidebarW, winPos.y + io.DisplaySize.y),
            IM_COL32(18, 18, 24, 255)
        );

        // Sidebar buttons - vertically centered between top bar and bottom
        float btnSize = 40.f;
        float btnPad = (sidebarW - btnSize) * 0.5f;
        float availableH = io.DisplaySize.y - 40 - 60; // below top bar (40px) minus logout area (60px)
        float totalBtnsH = btnSize * 2 + 20; // 2 buttons + gap
        float btnStartY = 40 + (availableH - totalBtnsH) * 0.5f; // start after top bar

        // Main button (home icon)
        ImGui::SetCursorPos(ImVec2(btnPad, btnStartY + dashOffsetY(1)));
        bool mainActive = (g_tab == Tab::NEWS || g_tab == Tab::FILES);
        ImGui::PushStyleColor(ImGuiCol_Button, mainActive ? ImVec4(0.08f, 0.08f, 0.12f, 1.f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.12f, 0.18f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, mainActive ? ImVec4(0.486f, 0.228f, 0.929f, 0.7f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, mainActive ? 1.f : 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        if (ImGui::Button("##mainbtn", ImVec2(btnSize, btnSize))) { g_tab = Tab::FILES; }
        ImVec2 mainBtnPos = ImGui::GetItemRectMin();
        ImGui::PushFont(io.Fonts->Fonts[3]);
        dlDash->AddText(ImVec2(mainBtnPos.x + 12, mainBtnPos.y + 12),
            mainActive ? IM_COL32(124, 58, 237, 255) : IM_COL32(80, 80, 100, 200), "\xee\x80\xb9");
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        // Profile button (person icon)
        ImGui::SetCursorPos(ImVec2(btnPad, btnStartY + btnSize + 20 + dashOffsetY(2)));
        bool profileActive = (g_tab == Tab::ACCOUNT);
        ImGui::PushStyleColor(ImGuiCol_Button, profileActive ? ImVec4(0.08f, 0.08f, 0.12f, 1.f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.12f, 0.18f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_Border, profileActive ? ImVec4(0.486f, 0.228f, 0.929f, 0.7f) : ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, profileActive ? 1.f : 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        if (ImGui::Button("##profilebtn", ImVec2(btnSize, btnSize))) { g_tab = Tab::ACCOUNT; }
        ImVec2 profBtnPos = ImGui::GetItemRectMin();
        ImGui::PushFont(io.Fonts->Fonts[3]);
        dlDash->AddText(ImVec2(profBtnPos.x + 12, profBtnPos.y + 12),
            profileActive ? IM_COL32(124, 58, 237, 255) : IM_COL32(80, 80, 100, 200), "\xee\x81\xb7");
        ImGui::PopFont();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        // Logout button (at bottom)
        float logoutY = io.DisplaySize.y - 60;
        ImGui::SetCursorPos(ImVec2(btnPad, logoutY));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.08f, 0.08f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        if (ImGui::Button("##logoutbtn", ImVec2(btnSize, btnSize))) {
            g_page = Page::LOGIN;
            g_loginInProgress = false;
            memset(g_user, 0, sizeof(g_user));
            memset(g_pass, 0, sizeof(g_pass));
        }
        ImVec2 logBtnPos = ImGui::GetItemRectMin();
        ImGui::PushFont(io.Fonts->Fonts[3]);
        dlDash->AddText(ImVec2(logBtnPos.x + 12, logBtnPos.y + 12), IM_COL32(140, 140, 160, 180), "\xee\x81\xab");
        ImGui::PopFont();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::PopStyleVar(); // dashAlpha(1)

        // ---- Main content area (right of sidebar) ----
        float contentX = sidebarW + 10.f;
        float contentY = 50.f;
        float contentW2 = io.DisplaySize.x - contentX - 10.f;
        float contentH = io.DisplaySize.y - contentY - 10.f;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, dashAlpha(2));
        ImGui::SetCursorPos(ImVec2(contentX, contentY + dashOffsetY(2)));

        // Content panel with subtle background
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.065f, 0.09f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
        ImGui::BeginChild("##mainContent", ImVec2(contentW2, contentH), true);

        if (g_tab == Tab::FILES || g_tab == Tab::NEWS) {
            // Main page - Modern card-based layout
            auto user = g_auth->getUser();
            auto app = g_auth->getApp();
            std::string hwid = GetHWID();

            // Top section - Welcome + Status badge
            ImGui::PushFont(io.Fonts->Fonts[1]);
            ImGui::Text("Hey, %s", user.username.c_str());
            ImGui::PopFont();
            ImGui::SameLine();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
            auto* dlContent = ImGui::GetWindowDrawList();
            ImVec2 badgePos = ImGui::GetCursorScreenPos();
            dlContent->AddRectFilled(badgePos, ImVec2(badgePos.x + 70, badgePos.y + 18), IM_COL32(30, 160, 80, 60), 9.f);
            dlContent->AddText(ImVec2(badgePos.x + 10, badgePos.y + 2), IM_COL32(80, 220, 130, 255), "Active");
            ImGui::Dummy(ImVec2(70, 18));

            ImGui::Spacing(); ImGui::Spacing();

            // Info cards row
            float cardW = (contentW2 - 50) / 2.f;
            float cardH = 75.f;

            // Card 1 - Subscription
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.06f, 0.14f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
            ImGui::BeginChild("##card1", ImVec2(cardW, cardH), true);
            ImGui::PushFont(io.Fonts->Fonts[2]);
            ImGui::TextColored(ImVec4(0.486f, 0.228f, 0.929f, 1.f), "Subscription");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Text("%s", user.subscription.empty() ? "Premium" : user.subscription.c_str());
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.5f, 1.f), "Expires: %s", user.expiresAt.empty() ? "Lifetime" : user.expiresAt.substr(0, 10).c_str());
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::SameLine(0, 10);

            // Card 2 - Security
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.10f, 0.08f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
            ImGui::BeginChild("##card2", ImVec2(cardW, cardH), true);
            ImGui::PushFont(io.Fonts->Fonts[2]);
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.4f, 1.f), "Security");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.2f, 0.85f, 0.4f, 1.f), "Undetected");
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.5f, 1.f), "HWID Lock + AES-256");
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            ImGui::Spacing(); ImGui::Spacing();

            // Details section
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.055f, 0.08f, 1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.f);
            ImGui::BeginChild("##details", ImVec2(contentW2 - 40, 0), true);

            ImGui::Spacing();
            ImGui::Columns(2, "##detcols", false);
            ImGui::SetColumnWidth(0, (contentW2 - 40) * 0.5f);

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "Username");
            ImGui::Text("%s", user.username.c_str());
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "Level");
            ImGui::Text("%d", user.level);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "HWID");
            ImGui::TextColored(ImVec4(0.35f, 0.35f, 0.45f, 1.f), "%s", hwid.c_str());

            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "Application");
            ImGui::Text("%s v%s", app.name.c_str(), app.version.c_str());
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "License Type");
            ImGui::Text("Premium");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.f), "Loader Version");
            ImGui::Text("1.0.0");

            ImGui::Columns(1);
            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

        } else if (g_tab == Tab::ACCOUNT) {
            auto user = g_auth->getUser();
            ImGui::PushFont(io.Fonts->Fonts[2]);
            ImGui::TextColored(ImVec4(0.655f, 0.545f, 0.980f, 1.f), "Account Settings");
            ImGui::PopFont();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Username: %s", user.username.c_str());
            ImGui::Text("Level: %d", user.level);
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.5f, 1.f), "Manage your account at authon.pro/dashboard");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(); // dashAlpha
    }

    ImGui::End();

    // Draw toast notification overlay
    DrawToast(io);
}

// ============ DX11 ============
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

bool CreateDevice(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, levels, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext)))
        return false;
    CreateRenderTarget();
    return true;
}

void LoadBannerTexture() {
    int w, h, channels;
    unsigned char* pixels = stbi_load_from_memory(Logo_data, Logo_size, &w, &h, &channels, 4);
    if (!pixels) return;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w; desc.Height = h;
    desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pixels;
    subResource.SysMemPitch = w * 4;

    ID3D11Texture2D* pTexture = nullptr;
    g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    if (pTexture) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &g_bannerTexture);
        pTexture->Release();
    }
    g_bannerW = w; g_bannerH = h;
    stbi_image_free(pixels);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    // Allow dragging from top bar area (y < 40px)
    if (msg == WM_NCHITTEST) {
        POINT pt; GetCursorPos(&pt); ScreenToClient(hWnd, &pt);
        if (pt.y < 40) {
            RECT rc; GetClientRect(hWnd, &rc);
            // Don't drag from button area (last 65px on right)
            if (pt.x < rc.right - 65) return HTCAPTION;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_auth = new authon::Authon(APP_ID, API_KEY, API_URL);
    if (!g_auth->init()) { MessageBoxA(NULL, "Failed to connect to Authon servers.", "Authon Error", MB_ICONERROR); return 1; }

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"AuthonLoaderWnd", NULL };
    RegisterClassExW(&wc);
    HWND hWnd = CreateWindowExW(WS_EX_APPWINDOW, wc.lpszClassName, WINDOW_TITLE, WS_POPUP | WS_VISIBLE,
        (GetSystemMetrics(SM_CXSCREEN) - WINDOW_WIDTH) / 2, (GetSystemMetrics(SM_CYSCREEN) - WINDOW_HEIGHT) / 2,
        WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, wc.hInstance, NULL);
    if (!CreateDevice(hWnd)) { MessageBoxA(NULL, "DirectX 11 init failed.", "Error", MB_ICONERROR); return 1; }

    // Round window corners (Windows 11)
    typedef HRESULT(WINAPI* DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwm = LoadLibraryA("dwmapi.dll");
    if (dwm) {
        auto pDwmSetWindowAttribute = (DwmSetWindowAttribute_t)GetProcAddress(dwm, "DwmSetWindowAttribute");
        if (pDwmSetWindowAttribute) {
            int val = 2; // DWMWCP_ROUND
            pDwmSetWindowAttribute(hWnd, 33, &val, sizeof(val)); // DWMWA_WINDOW_CORNER_PREFERENCE
        }
    }

    LoadBannerTexture();
    ShowWindow(hWnd, SW_SHOWDEFAULT); UpdateWindow(hWnd);

    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& imguiIO = ImGui::GetIO(); imguiIO.IniFilename = nullptr;

    char winDir[MAX_PATH]; GetWindowsDirectoryA(winDir, MAX_PATH);
    std::string fontNormal = std::string(winDir) + "\\Fonts\\segoeui.ttf";
    std::string fontBold = std::string(winDir) + "\\Fonts\\segoeuib.ttf";
    imguiIO.Fonts->AddFontFromFileTTF(fontNormal.c_str(), 13.0f); // [0] Normal
    imguiIO.Fonts->AddFontFromFileTTF(fontBold.c_str(), 18.0f);   // [1] Title
    imguiIO.Fonts->AddFontFromFileTTF(fontBold.c_str(), 12.0f);   // [2] Bold small

    // [3] Icon font - Segoe MDL2 Assets (Windows built-in icons)
    std::string iconFont = std::string(winDir) + "\\Fonts\\segmdl2.ttf";
    static const ImWchar iconRanges[] = { 0xE000, 0xF000, 0 };
    ImFontConfig iconCfg;
    iconCfg.MergeMode = false;
    iconCfg.PixelSnapH = true;
    imguiIO.Fonts->AddFontFromFileTTF(iconFont.c_str(), 14.0f, &iconCfg, iconRanges); // [3] Icons

    SetTheme();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); continue; }
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        RenderUI();
        ImGui::EndFrame(); ImGui::Render();
        const float clear[4] = { 0.055f, 0.055f, 0.07f, 1.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
    DestroyWindow(hWnd); UnregisterClassW(wc.lpszClassName, wc.hInstance);
    delete g_auth; return 0;
}
