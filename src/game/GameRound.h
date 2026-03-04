#pragma once
#include <SDL2/SDL.h>
#include "audio/SFX.h"
#include "game/ResultScreen.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include "game/Fish.h"
#include "game/Diamond.h"

// Game configuration for a single round (mirrors HTML mode tier config)
struct RoundConfig {
    int   fishCount   = 20;   // number of fish
    float trackTime   = 30.0f; // seconds to track
    float speed       = 1.0f;  // fish speed multiplier
    int   diamondCount = 1;    // how many diamond fish
};

enum class GamePhase { Idle, Observe, Track, Select, Reveal };

struct OverlayMessage {
    std::string text;
    bool        visible = false;
    float       timeLeft = 0;   // seconds; -1 = permanent until manually hidden
};

class GameRound {
public:
    GamePhase   phase      = GamePhase::Idle;
    float       timer      = 0;   // countdown (seconds)
    bool        won        = false;
    bool        lost       = false;
    bool        hintUsed_  = false;   // public for HUD display

    std::vector<Fish> fishes;
    std::vector<int>  diamondFishIdx;
    DiamondDrop       diamond;
    OverlayMessage    overlay;

    // Spread phase state (mirrors HTML startSpread / updateSpread)
    bool  isSpreadingOut  = false;
    bool  spreadComplete  = false;
    bool  canSelect       = false;

    // Callback fired when the round ends.
    // Parameters: won, timeTaken (seconds)
    std::function<void(bool won, float timeTaken)> onRoundEnd;

    // ---- Public API ----

    void startRound(const RoundConfig& cfg, int canvasW, int canvasH) {
        config_    = cfg;
        canvasW_   = canvasW;
        canvasH_   = canvasH;
        won = lost = false;
        isSpreadingOut = spreadComplete = canSelect = false;
        diamondFishIdx.clear();
        fishes.clear();
        diamond.hide();
        hintUsed_      = false;
        swallowPending_ = false;
        swallowTimer_   = 0;
        postSwallowTimer_ = 0;
        observeTimer_   = 3.0f + (cfg.diamondCount - 1) * 1.5f;

        // Spawn fish - single species (classic mode equivalent)
        FishType  specType  = randomFishType();

        for (int i = 0; i < cfg.fishCount; i++) {
            float fx = 80.0f + (float)(rand() % (canvasW - 160));
            float fy = 80.0f + (float)(rand() % (canvasH - 160));
            const auto& cols = fishColors(specType);
            std::string col  = cols[rand() % cols.size()];
            Fish f(fx, fy, specType, col);
            f.speedMultiplier = cfg.speed;
            fishes.push_back(f);
        }

        // Assign diamond fish (exclude lazy personality like HTML version)
        for (int d = 0; d < cfg.diamondCount; d++) {
            int idx;
            do { idx = rand() % (int)fishes.size(); }
            while (std::find(diamondFishIdx.begin(), diamondFishIdx.end(), idx)
                   != diamondFishIdx.end());
            diamondFishIdx.push_back(idx);
            fishes[idx].hasDiamond = true;
            if (fishes[idx].personality == Personality::Lazy)
                fishes[idx].personality = Personality::Explorer;
            fishes[idx].diamondSpeedBoost = 1.1f + (float)(rand() % 10) / 100.0f;
        }

        // Start observe phase: drop first diamond
        phase = GamePhase::Observe;
        showOverlay("OBSERVE!", 99.0f);
        dropDiamond(0);
    }

    // Call every frame. dt = milliseconds since last frame.
    void update(float dt) {
        if (phase == GamePhase::Idle) return;

        overlay.timeLeft -= dt / 1000.0f;
        if (overlay.timeLeft <= 0 && overlay.timeLeft != -99.0f)
            overlay.visible = false;

        const Bounds bounds = {0, (float)canvasW_, 0, (float)canvasH_};

        switch (phase) {
        case GamePhase::Observe:
            updateObserve(dt);
            break;

        case GamePhase::Track:
            timer -= dt / 1000.0f;
            for (auto& f : fishes) f.update(fishes, bounds, dt);
            // Tick sounds for last 5 seconds
            {
                int prevSec = (int)ceilf(timer + dt / 1000.0f);
                int currSec = (int)ceilf(timer);
                if (prevSec != currSec && currSec <= 5 && currSec > 0) {
                    if (currSec <= 2) SFX.tickUrgent();
                    else              SFX.tick();
                }
            }
            if (timer <= 0.0f) {
                timer = 0;
                phase = GamePhase::Select;
                SFX.timeUp();
                SFX.stopAmbient();
                startSpread();
            }
            break;

        case GamePhase::Select:
            updateSelect(dt);
            break;

        case GamePhase::Reveal:
            // Just wait — round is over
            break;

        default: break;
        }
    }

    // Call with normalized [0,1] coordinates. Returns true if click was consumed.
    bool handleClick(float normX, float normY) {
        if (phase == GamePhase::Track) {
            trackPhaseClicks_++;
            return false;
        }
        if (phase != GamePhase::Select || !canSelect) return false;

        float px = normX * canvasW_;
        float py = normY * canvasH_;

        for (int i = 0; i < (int)fishes.size(); i++) {
            if (fishes[i].contains(px, py)) {
                selectFish(i);
                return true;
            }
        }
        return false;
    }

    void useHint() { hintUsed_ = true; }
    bool hintUsed() const { return hintUsed_; }

    // Draw everything: fish, diamond, overlay text
    void draw(SDL_Renderer* rnd, float now) const {
        // Fish
        SDL_RenderSetClipRect(rnd, nullptr);
        for (const auto& f : fishes) f.draw(rnd);

        // Diamond
        diamond.draw(rnd);

        // Hint highlight - visible from the moment H is pressed (track + select)
        if ((phase == GamePhase::Track || phase == GamePhase::Select) && hintUsed_) {
            SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
            for (int idx : diamondFishIdx) {
                const auto& f = fishes[idx];
                float alpha = 0.25f + sinf(now * 0.004f) * 0.15f;
                SDL_SetRenderDrawColor(rnd, 255, 176, 0, (uint8_t)(alpha * 255));
                for (int r = 33; r <= 36; r++) {
                    SDL_Rect ring = {(int)f.x - r, (int)f.y - r, r*2, r*2};
                    SDL_RenderDrawRect(rnd, &ring);
                }
            }
            SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
        }
    }

    // Draw the HUD overlay text (call after fish/diamond draw)
    void drawOverlay(SDL_Renderer* rnd, int winW, int winH) const {
        if (!overlay.visible || overlay.text.empty()) return;
        // Draw a semi-transparent dark banner in the center
        SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(rnd, 0, 0, 20, 180);
        SDL_Rect banner = {winW/2 - 160, winH/2 - 30, 320, 60};
        SDL_RenderFillRect(rnd, &banner);
        // Cyan border
        SDL_SetRenderDrawColor(rnd, 0, 229, 255, 200);
        SDL_RenderDrawRect(rnd, &banner);
        SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
        // Note: text rendering requires SDL_ttf which is not yet wired up.
        // For now the overlay is a visible banner; text will be added later.
    }

private:
    RoundConfig config_;
    int   canvasW_ = 800, canvasH_ = 600;
    bool  swallowPending_ = false;
    float swallowTimer_   = 0;
    float postSwallowTimer_ = 0;
    float observeTimer_   = 3.0f;
    int   currentDiamondDrop_ = 0;
    int   trackPhaseClicks_   = 0;
    float revealTimer_    = 0;
    float selectStartTime_ = 0.0f;   // SDL_GetTicks() when select phase begins

    void showOverlay(const std::string& txt, float duration = 1.5f) {
        overlay.text    = txt;
        overlay.visible = true;
        overlay.timeLeft = duration;
    }
    void hideOverlay() { overlay.visible = false; }

    void dropDiamond(int dropIndex) {
        if (dropIndex >= (int)diamondFishIdx.size()) return;
        currentDiamondDrop_ = dropIndex;
        int   idx = diamondFishIdx[dropIndex];
        const auto& tf = fishes[idx];
        float offsetX = (dropIndex - (config_.diamondCount - 1) * 0.5f) * 60.0f;
        diamond.reset(tf.x + offsetX, tf.y);
        diamond.targetFishIdx = idx;
    }

    void updateObserve(float dt) {
        observeTimer_ -= dt / 1000.0f;

        bool arrived = diamond.update();

        if (arrived && !swallowPending_) {
            swallowPending_ = true;
            swallowTimer_   = 0.4f;
            int idx = diamond.targetFishIdx;
            fishes[idx].isSwallowing  = true;
            fishes[idx].swallowFrame  = 0;
            SFX.swallow();
        }

        if (swallowPending_) {
            swallowTimer_ -= dt / 1000.0f;
            if (swallowTimer_ <= 0) {
                swallowPending_  = false;
                diamond.hide();
                postSwallowTimer_ = 0.4f;
            }
        }

        if (!swallowPending_ && !diamond.visible && postSwallowTimer_ > 0) {
            postSwallowTimer_ -= dt / 1000.0f;
            if (postSwallowTimer_ <= 0) {
                currentDiamondDrop_++;
                if (currentDiamondDrop_ < config_.diamondCount) {
                    dropDiamond(currentDiamondDrop_);
                    swallowPending_ = false;
                } else {
                    enterTrack();
                }
            }
        }
    }

    void enterTrack() {
        phase = GamePhase::Track;
        timer = config_.trackTime;
        showOverlay("TRACK!", 0.8f);
        SFX.startAmbient();
    }

    void startSpread() {
        isSpreadingOut = true;
        selectStartTime_ = (float)SDL_GetTicks();
        SFX.spread();
        spreadComplete = false;
        canSelect      = false;

        // Calculate grid positions (mirrors HTML startSpread exactly)
        const int    padding = 60;
        const float  aspect  = (float)canvasW_ / (float)canvasH_;
        const int    cols    = (int)ceilf(sqrtf((float)fishes.size() * aspect));
        const int    rows    = (int)ceilf((float)fishes.size() / cols);
        const float  cellW   = (canvasW_ - padding * 2.0f) / cols;
        const float  cellH   = (canvasH_ - padding * 2.0f) / rows;

        // Build shuffled target positions
        struct Pos { float x, y; };
        std::vector<Pos> positions;
        for (int i = 0; i < (int)fishes.size(); i++) {
            int col = i % cols;
            int row = i / cols;
            positions.push_back({
                padding + cellW * (col + 0.5f) + ((float)(rand() % 100)/100.0f - 0.5f) * cellW * 0.3f,
                padding + cellH * (row + 0.5f) + ((float)(rand() % 100)/100.0f - 0.5f) * cellH * 0.3f
            });
        }
        // Fisher-Yates shuffle
        for (int i = (int)positions.size() - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(positions[i], positions[j]);
        }

        for (int i = 0; i < (int)fishes.size(); i++) {
            fishes[i].spreadTarget = {positions[i].x, positions[i].y};
            fishes[i].isSpreading  = true;
        }
    }

    void updateSelect(float dt) {
        if (isSpreadingOut) {
            bool allArrived = true;
            for (auto& f : fishes) {
                if (!f.isSpreading) continue;
                float dx   = f.spreadTarget.x - f.x;
                float dy   = f.spreadTarget.y - f.y;
                float dist = sqrtf(dx*dx + dy*dy);
                if (dist < 5.0f) {
                    f.isSpreading = false;
                    f.vx = ((float)(rand()%100)/100.0f - 0.5f) * 0.3f;
                    f.vy = ((float)(rand()%100)/100.0f - 0.5f) * 0.3f;
                } else {
                    allArrived = false;
                    float speed = 2.5f;
                    f.vx = (dx / dist) * speed;
                    f.vy = (dy / dist) * speed;
                    f.x += f.vx;
                    f.y += f.vy;
                    if (fabsf(f.vx) > 0.1f) f.targetDir = f.vx > 0 ? 1 : -1;
                }
                f.frame += dt * 0.06f;
            }
            if (allArrived) {
                isSpreadingOut = false;
                spreadComplete = true;
                canSelect      = true;
                showOverlay("FIND THE DIAMOND FISH!", 1.2f);
            }
        } else {
            // Gentle float after spread
            for (auto& f : fishes) {
                f.frame += dt * 0.06f;
                f.x += f.vx;
                f.y += f.vy;
                f.vx *= 0.98f;
                f.vy *= 0.98f;
                if (f.x < 40) f.vx += 0.1f;
                if (f.x > canvasW_ - 40) f.vx -= 0.1f;
                if (f.y < 40) f.vy += 0.1f;
                if (f.y > canvasH_ - 40) f.vy -= 0.1f;
            }
        }
    }

    void selectFish(int idx) {
        phase = GamePhase::Reveal;
        bool correct = std::find(diamondFishIdx.begin(), diamondFishIdx.end(), idx)
                       != diamondFishIdx.end();

        if (correct) {
            fishes[idx].revealing = true;
            SFX.correct();
            diamondFishIdx.erase(
                std::remove(diamondFishIdx.begin(), diamondFishIdx.end(), idx),
                diamondFishIdx.end());

            if (!diamondFishIdx.empty()) {
                // More diamond fish to find
                showOverlay("FOUND ONE! KEEP GOING!", 1.0f);
                phase      = GamePhase::Select;
                canSelect  = true;
            } else {
                // All found!
                diamond.x       = fishes[idx].x;
                diamond.y       = fishes[idx].y;
                diamond.visible = true;
                won = true;
                showOverlay("EXCELLENT!", 99.0f);
                float timeTaken = ((float)SDL_GetTicks() - selectStartTime_) / 1000.0f;
                if (onRoundEnd) onRoundEnd(true, timeTaken);
            }
        } else {
            SFX.wrong();
            // Wrong fish - reveal all remaining diamond fish
            for (int ri : diamondFishIdx) {
                fishes[ri].revealing = true;
                diamond.x = fishes[ri].x;
                diamond.y = fishes[ri].y;
                diamond.visible = true;
            }
            lost = true;
            showOverlay("WRONG FISH!", 99.0f);
            if (onRoundEnd) onRoundEnd(false, 0.0f);
        }
    }
};
