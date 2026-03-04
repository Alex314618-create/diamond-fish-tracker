// Diamond Fish Tracker - Phase 6
// Result screen + streak system

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
#include "game/ResultScreen.h"
#include "render/TextRenderer.h"
#include "audio/SFX.h"

constexpr int WINDOW_W = 800;
constexpr int WINDOW_H = 600;
constexpr int TANK_W   = WINDOW_W;
constexpr int TANK_H   = WINDOW_H - 80;
constexpr const char* WINDOW_TITLE = "Diamond Fish Tracker";

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

static RoundConfig getConfig(int difficulty) {
    const int   fishCounts[] = {0,20,22,25,28,32,36,40,44,47,50};
    const float trackTimes[] = {0,60,65,70,75,80,90,100,110,115,120};
    const float speeds[]     = {0,1.0f,1.0f,1.05f,1.1f,1.15f,1.2f,1.25f,1.3f,1.35f,1.4f};
    int d = std::clamp(difficulty, 1, 10);
    return {fishCounts[d], trackTimes[d], speeds[d], 1};
}

int main(int argc, char* argv[]) {
    srand((unsigned)time(nullptr));

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ApplyCassetteTheme();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    GestureEngine gesture;
    gesture.start([](const InputEvent&) {});

    TextRenderer text;
    text.init(renderer, "assets/fonts");

    SFX.init();

    // ── Game state ────────────────────────────────────────────────────────
    GameRound    round;
    ResultScreen result;
    int   difficulty   = 1;
    int   wins         = 0;
    int   losses       = 0;
    int   streak       = 0;
    bool  roundActive  = false;

    auto startNewRound = [&]() {
        result.hide();
        SFX.splash();
        RoundConfig cfg = getConfig(difficulty);
        round.onRoundEnd = [&](bool won, float timeTaken) {
            roundActive = false;

            if (won) { wins++; streak++; }
            else     { losses++; streak = 0; }

            ResultData rd;
            rd.won         = won;
            rd.difficulty  = difficulty;
            rd.timeTaken   = timeTaken;
            rd.streak      = streak;
            rd.totalWins   = wins;
            rd.totalLosses = losses;

            result.show(rd);

            // Auto-adjust difficulty suggestion
            if (won && difficulty < 10) difficulty++;
        };
        round.startRound(cfg, TANK_W, TANK_H);
        roundActive = true;
    };

    Uint32 lastTick = SDL_GetTicks();
    bool   running  = true;
    SDL_Event event;

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = std::min((float)(now - lastTick), 50.0f);
        lastTick   = now;

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: running = false; break;
                case SDLK_SPACE:
                    if (!roundActive && !result.visible) startNewRound();
                    break;
                case SDLK_r:
                    startNewRound();
                    break;
                case SDLK_h:
                    if (roundActive) { round.useHint(); SFX.hint(); }
                    break;
                }
            }

            if (!io.WantCaptureMouse) {
                if (event.type == SDL_MOUSEMOTION) {
                    result.handleMouseMove(event.motion.x, event.motion.y,
                                           WINDOW_W, WINDOW_H);
                }
                if (event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {

                    // Result screen button takes priority
                    if (result.visible) {
                        if (result.handleClick(event.button.x, event.button.y,
                                               WINDOW_W, WINDOW_H)) {
                            startNewRound();
                        }
                    } else if (roundActive) {
                        float nx = (float)event.button.x / TANK_W;
                        float ny = (float)event.button.y / TANK_H;
                        round.handleClick(nx, ny);
                    }
                }
            }
        }

        // ── Update ────────────────────────────────────────────────────────
        if (roundActive) round.update(dt);
        result.update(dt);

        // ── ImGui ─────────────────────────────────────────────────────────
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // HUD bar (always visible)
        ImGui::SetNextWindowPos(ImVec2(0, WINDOW_H - 80), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2((float)WINDOW_W, 80), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("HUD", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);

        if (roundActive && round.phase == GamePhase::Track) {
            int secs = (int)ceilf(round.timer);
            ImVec4 col = secs <= 5
                ? ImVec4(1,0.2f,0.2f,1) : ImVec4(0,0.9f,1,1);
            ImGui::TextColored(col, "TIME  %02d", secs);
        } else {
            ImGui::TextColored(ImVec4(0.5f,0.5f,0.7f,1), "TIME  --");
        }
        ImGui::SameLine(120);
        ImGui::TextColored(ImVec4(1,0.7f,0,1), "FISH  %d",
            roundActive ? (int)round.fishes.size() : 0);
        ImGui::SameLine(240);
        ImGui::TextColored(ImVec4(0.4f,1,0.3f,1), "W:%d  L:%d  STK:%d",
            wins, losses, streak);
        ImGui::SameLine(420);
        ImGui::TextColored(ImVec4(0.7f,0.5f,1,1), "LV %d", difficulty);
        ImGui::SameLine(500);
        if (!roundActive && !result.visible)
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1),
                               "SPACE=start  R=restart");
        else
            ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1),
                               "R=restart  H=hint");
        ImGui::SameLine((float)WINDOW_W - 140);
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("Vol", &SFX.masterVolume, 0.0f, 1.0f);

        ImGui::End();
        ImGui::PopStyleVar();

        // Start panel (idle, no result screen)
        if (!roundActive && !result.visible) {
            ImGui::SetNextWindowPos(
                ImVec2(WINDOW_W/2 - 150, WINDOW_H/2 - 60), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Always);
            ImGui::Begin("Start", nullptr,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoSavedSettings);
            ImGui::TextColored(ImVec4(0,0.9f,1,1), "DIAMOND FISH TRACKER");
            ImGui::Separator();
            ImGui::SliderInt("Difficulty", &difficulty, 1, 10);
            if (ImGui::Button("START  ( SPACE )", ImVec2(-1, 0)))
                startNewRound();
            ImGui::End();
        }

        // ── SDL2 render ───────────────────────────────────────────────────
        SDL_SetRenderDrawColor(renderer, 10, 10, 26, 255);
        SDL_RenderClear(renderer);

        // Tank background
        SDL_SetRenderDrawColor(renderer, 8, 20, 50, 255);
        SDL_Rect tankRect = {0, 0, TANK_W, TANK_H};
        SDL_RenderFillRect(renderer, &tankRect);

        // Fish + diamond (always draw so they show behind result screen)
        SDL_RenderSetClipRect(renderer, &tankRect);
        if (roundActive || result.visible)
            round.draw(renderer, (float)now);
        SDL_RenderSetClipRect(renderer, nullptr);

        // Overlay banner (phase messages)
        if (roundActive && round.overlay.visible && !round.overlay.text.empty()) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 20, 180);
            SDL_Rect banner = {WINDOW_W/2-200, WINDOW_H/2-36, 400, 72};
            SDL_RenderFillRect(renderer, &banner);
            SDL_SetRenderDrawColor(renderer, 0, 229, 255, 220);
            SDL_RenderDrawRect(renderer, &banner);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            SDL_Color col = {0, 229, 255, 255};
            const auto& t = round.overlay.text;
            if (t.find("EXCELLENT") != std::string::npos) col = {100,255,80,255};
            else if (t.find("WRONG")  != std::string::npos) col = {255,80,80,255};
            else if (t.find("TRACK")  != std::string::npos) col = {255,179,0,255};
            text.drawCentered(t, WINDOW_W/2, WINDOW_H/2, col, 28);
        }

        // Hint indicator
        if (roundActive && round.hintUsed_) {
            SDL_Color amber = {255, 179, 0, 255};
            text.draw("HINT ACTIVE", WINDOW_W - 140, 10, amber, 14);
        }

        // Tank border
        SDL_SetRenderDrawColor(renderer, 0, 229, 255, 255);
        SDL_RenderDrawRect(renderer, &tankRect);

        // HUD bar background
        SDL_SetRenderDrawColor(renderer, 15, 8, 35, 255);
        SDL_Rect hudRect = {0, WINDOW_H-80, WINDOW_W, 80};
        SDL_RenderFillRect(renderer, &hudRect);
        SDL_SetRenderDrawColor(renderer, 255, 179, 0, 255);
        SDL_RenderDrawLine(renderer, 0, WINDOW_H-80, WINDOW_W, WINDOW_H-80);

        // Result screen (on top of everything, below ImGui)
        result.draw(renderer, text, WINDOW_W, WINDOW_H);

        // ImGui on top of all
        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    gesture.stop();
    SFX.destroy();
    text.destroy();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
