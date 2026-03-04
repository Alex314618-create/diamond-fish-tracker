// Diamond Fish Tracker - Phase 3
// Core game loop: observe -> track -> select -> reveal

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

#include "core/InputEvent.h"
#include "gesture/GestureEngine.h"
#include "game/GameRound.h"
#include "render/TextRenderer.h"
#include "audio/SFX.h"

// ---- Layout constants ----
constexpr int WINDOW_W = 800;
constexpr int WINDOW_H = 600;
constexpr int TANK_X   = 0;
constexpr int TANK_Y   = 0;
constexpr int TANK_W   = WINDOW_W;
constexpr int TANK_H   = WINDOW_H - 80;  // HUD at bottom
constexpr const char* WINDOW_TITLE = "Diamond Fish Tracker";

// ---- ImGui Cassette theme ----
static void ApplyCassetteTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 4.0f; s.FrameRounding = 2.0f;
    s.WindowBorderSize = 1.0f; s.FrameBorderSize = 1.0f;
    s.WindowPadding = ImVec2(12,12); s.FramePadding = ImVec2(8,4);
    s.ItemSpacing = ImVec2(8,6);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.06f,0.06f,0.15f,0.95f);
    c[ImGuiCol_Border]           = ImVec4(0.00f,0.90f,1.00f,0.40f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.10f,0.10f,0.25f,1.00f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.15f,0.15f,0.35f,1.00f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.49f,0.30f,1.00f,0.60f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.04f,0.04f,0.10f,1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.08f,0.08f,0.20f,1.00f);
    c[ImGuiCol_Button]           = ImVec4(0.10f,0.10f,0.28f,1.00f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.00f,0.90f,1.00f,0.20f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.00f,0.90f,1.00f,0.50f);
    c[ImGuiCol_Tab]              = ImVec4(0.08f,0.08f,0.20f,1.00f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.00f,0.90f,1.00f,0.30f);
    c[ImGuiCol_TabActive]        = ImVec4(0.00f,0.90f,1.00f,0.20f);
    c[ImGuiCol_SliderGrab]       = ImVec4(1.00f,0.70f,0.00f,1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.00f,0.90f,0.20f,1.00f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.41f,1.00f,0.28f,1.00f);
    c[ImGuiCol_Text]             = ImVec4(0.90f,0.90f,1.00f,1.00f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.40f,0.40f,0.60f,1.00f);
    c[ImGuiCol_Separator]        = ImVec4(0.00f,0.90f,1.00f,0.25f);
    c[ImGuiCol_ScrollbarBg]      = ImVec4(0.04f,0.04f,0.10f,1.00f);
    c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.20f,0.20f,0.40f,1.00f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.06f,0.06f,0.16f,0.98f);
    c[ImGuiCol_Header]           = ImVec4(0.49f,0.30f,1.00f,0.40f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.49f,0.30f,1.00f,0.60f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.49f,0.30f,1.00f,0.80f);
}

// ---- Round config presets ----
static RoundConfig getConfig(int difficulty) {
    const int   fishCounts[] = {0, 20, 22, 25, 28, 32, 36, 40, 44, 47, 50};
    const float trackTimes[] = {0, 60, 65, 70, 75, 80, 90,100,110,115,120};
    const float speeds[]     = {0,1.0f,1.0f,1.05f,1.1f,1.15f,1.2f,1.25f,1.3f,1.35f,1.4f};
    int d = std::clamp(difficulty, 1, 10);
    RoundConfig cfg;
    cfg.fishCount    = fishCounts[d];
    cfg.trackTime    = trackTimes[d];
    cfg.speed        = speeds[d];
    cfg.diamondCount = 1;
    return cfg;
}

int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));

    // ---- SDL2 ----
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    SDL_Window* window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // ---- ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ApplyCassetteTheme();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // ---- GestureEngine ----
    GestureEngine gesture;
    gesture.start([](const InputEvent&) {});

    // ---- Text renderer ----
    TextRenderer text;
    text.init(renderer, "assets/fonts");
    SFX.init();

    // ---- Game state ----
    GameRound round;
    int  difficulty  = 1;
    int  wins        = 0;
    int  losses      = 0;
    bool roundActive = false;

    auto startNewRound = [&]() {
        SFX.splash();
        RoundConfig cfg = getConfig(difficulty);
        round.onRoundEnd = [&](bool w) {
            if (w) wins++;
            else   losses++;
            roundActive = false;
        };
        round.startRound(cfg, TANK_W, TANK_H);
        roundActive = true;
    };

    // ---- Timing ----
    Uint32 lastTick = SDL_GetTicks();

    std::cout << "[Phase 3] Core game loop ready. Press SPACE to start.\n";

    // ---- Main loop ----
    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = std::min((float)(now - lastTick), 50.0f);
        lastTick   = now;

        // -- Events --
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE:
                    if (!roundActive) startNewRound();
                    break;
                case SDLK_r:
                    startNewRound();
                    break;
                case SDLK_h:
                    if (roundActive) {
                        round.useHint();
                        SFX.hint();
                    }
                    break;
                }
            }

            // Mouse click -> game input (only when ImGui not capturing)
            if (!io.WantCaptureMouse && event.type == SDL_MOUSEBUTTONDOWN
                && event.button.button == SDL_BUTTON_LEFT) {
                float nx = (float)event.button.x / TANK_W;
                float ny = (float)event.button.y / TANK_H;
                if (roundActive) round.handleClick(nx, ny);
            }
        }

        // -- Update game --
        if (roundActive) round.update(dt);

        // -- ImGui HUD panel --
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // HUD: bottom bar
        ImGui::SetNextWindowPos(ImVec2(0, (float)(WINDOW_H - 80)), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)WINDOW_W, 80), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("HUD", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);

        // Timer
        if (roundActive && round.phase == GamePhase::Track) {
            int secs = (int)ceilf(round.timer);
            ImVec4 timerCol = (secs <= 5)
                ? ImVec4(1,0.2f,0.2f,1)
                : ImVec4(0,0.9f,1,1);
            ImGui::TextColored(timerCol, "TIME  %02d", secs);
        } else {
            ImGui::TextColored(ImVec4(0.5f,0.5f,0.7f,1), "TIME  --");
        }
        ImGui::SameLine(120);

        // Fish count
        ImGui::TextColored(ImVec4(1,0.7f,0,1), "FISH  %d",
            roundActive ? (int)round.fishes.size() : 0);
        ImGui::SameLine(240);

        // Score
        ImGui::TextColored(ImVec4(0.4f,1,0.3f,1), "W:%d  L:%d", wins, losses);
        ImGui::SameLine(380);

        // Phase indicator
        const char* phaseStr = "IDLE";
        if (roundActive) {
            switch (round.phase) {
            case GamePhase::Observe: phaseStr = "OBSERVE"; break;
            case GamePhase::Track:   phaseStr = "TRACK";   break;
            case GamePhase::Select:  phaseStr = "SELECT";  break;
            case GamePhase::Reveal:  phaseStr = "REVEAL";  break;
            default: break;
            }
        }
        ImGui::TextColored(ImVec4(0.7f,0.5f,1,1), "%s", phaseStr);
        ImGui::SameLine(500);

        // Controls hint
        if (!roundActive) {
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1), "SPACE=start  R=restart  H=hint");
            ImGui::SameLine(500);
            ImGui::SetNextItemWidth(120);
            if (ImGui::SliderFloat("Vol", &SFX.masterVolume, 0.0f, 1.0f))
                {} // value updates live via masterVolume reference
        }
        else
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1), "R=restart  H=hint  ESC=quit");

        ImGui::SameLine((float)WINDOW_W - 100);
        ImGui::TextColored(ImVec4(0.4f,0.4f,0.6f,1), "FPS %.0f", io.Framerate);

        ImGui::End();
        ImGui::PopStyleVar();

        // Difficulty selector (only when idle)
        if (!roundActive) {
            ImGui::SetNextWindowPos(ImVec2((float)(WINDOW_W/2 - 150), (float)(WINDOW_H/2 - 60)),
                                    ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
            ImGui::Begin("Start Game", nullptr,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings);
            ImGui::TextColored(ImVec4(0,0.9f,1,1), "DIAMOND FISH TRACKER");
            ImGui::Separator();
            ImGui::SliderInt("Difficulty", &difficulty, 1, 10);
            if (ImGui::Button("START ROUND (SPACE)", ImVec2(-1, 0)))
                startNewRound();
            ImGui::End();
        }

        // -- SDL2 render --
        SDL_SetRenderDrawColor(renderer, 10, 10, 26, 255);
        SDL_RenderClear(renderer);

        // Tank background
        SDL_SetRenderDrawColor(renderer, 8, 20, 50, 255);
        SDL_Rect tankRect = {TANK_X, TANK_Y, TANK_W, TANK_H};
        SDL_RenderFillRect(renderer, &tankRect);

        // Clip fish to tank
        SDL_RenderSetClipRect(renderer, &tankRect);
        if (roundActive) round.draw(renderer, (float)now);
        SDL_RenderSetClipRect(renderer, nullptr);

        // Overlay banner with text
        if (roundActive && round.overlay.visible && !round.overlay.text.empty()) {
            // Background banner
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 20, 180);
            SDL_Rect banner = {WINDOW_W/2 - 200, WINDOW_H/2 - 36, 400, 72};
            SDL_RenderFillRect(renderer, &banner);
            // Cyan border
            SDL_SetRenderDrawColor(renderer, 0, 229, 255, 220);
            SDL_RenderDrawRect(renderer, &banner);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            // Text centered in banner
            SDL_Color cyan  = {0, 229, 255, 255};
            SDL_Color amber = {255, 179, 0, 255};
            SDL_Color green = {100, 255, 80, 255};
            SDL_Color red   = {255, 80,  80, 255};
            // Choose color by content
            SDL_Color col = cyan;
            if (round.overlay.text.find("EXCELLENT") != std::string::npos) col = green;
            else if (round.overlay.text.find("WRONG")    != std::string::npos) col = red;
            else if (round.overlay.text.find("TRACK")    != std::string::npos) col = amber;
            text.drawCentered(round.overlay.text, WINDOW_W/2, WINDOW_H/2, col, 28);
        }

        // Hint active indicator (top-right corner)
        if (roundActive && round.hintUsed_) {
            SDL_Color amber = {255, 179, 0, 255};
            text.draw("HINT ACTIVE", WINDOW_W - 140, 10, amber, 14);
        }

        // Tank border (cyan)
        SDL_SetRenderDrawColor(renderer, 0, 229, 255, 255);
        SDL_RenderDrawRect(renderer, &tankRect);

        // HUD background bar
        SDL_SetRenderDrawColor(renderer, 15, 8, 35, 255);
        SDL_Rect hudRect = {0, WINDOW_H - 80, WINDOW_W, 80};
        SDL_RenderFillRect(renderer, &hudRect);
        SDL_SetRenderDrawColor(renderer, 255, 179, 0, 255);
        SDL_RenderDrawLine(renderer, 0, WINDOW_H - 80, WINDOW_W, WINDOW_H - 80);

        // ImGui on top
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // -- Cleanup --
    gesture.stop();
    SFX.destroy();
    text.destroy();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::cout << "[Phase 3] Clean exit.\n";
    return 0;
}
