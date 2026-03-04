#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <cmath>
#include "game/Color.h"
#include "render/TextRenderer.h"
#include "audio/SFX.h"

// ── ResultData: filled by GameRound when round ends ───────────────────────
struct ResultData {
    bool  won          = false;
    int   difficulty   = 1;
    float timeTaken    = 0.0f;   // seconds from select-start to correct click
    int   streak       = 0;      // current win streak (updated before show)
    int   totalWins    = 0;
    int   totalLosses  = 0;
    // Phase 8 will add: int silverEarned, goldEarned, diamondEarned, soulSand;
};

// ── ResultScreen ──────────────────────────────────────────────────────────
class ResultScreen {
public:
    bool visible = false;

    void show(const ResultData& data) {
        data_        = data;
        visible      = true;
        animTimer_   = 0.0f;
        fadeAlpha_   = 0.0f;
        btnHovered_  = false;
    }

    void hide() {
        visible    = false;
        animTimer_ = 0.0f;
    }

    // dt in milliseconds
    void update(float dt) {
        if (!visible) return;
        animTimer_ += dt / 1000.0f;
        // Fade in over 0.4s
        fadeAlpha_ = std::min(1.0f, animTimer_ / 0.4f);
    }

    // Returns true when "Play Again" button is clicked
    bool handleClick(int mx, int my, int winW, int winH) {
        if (!visible) return false;
        // Button rect (centered, bottom third)
        SDL_Rect btn = buttonRect(winW, winH);
        if (mx >= btn.x && mx < btn.x + btn.w &&
            my >= btn.y && my < btn.y + btn.h) {
            hide();
            return true;
        }
        return false;
    }

    void handleMouseMove(int mx, int my, int winW, int winH) {
        if (!visible) return;
        SDL_Rect btn = buttonRect(winW, winH);
        btnHovered_ = (mx >= btn.x && mx < btn.x + btn.w &&
                       my >= btn.y && my < btn.y + btn.h);
    }

    void draw(SDL_Renderer* rnd, TextRenderer& text, int winW, int winH) {
        if (!visible) return;

        uint8_t overlayAlpha = (uint8_t)(fadeAlpha_ * 210);

        // ── Dark overlay ──────────────────────────────────────────────────
        SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(rnd, 4, 4, 18, overlayAlpha);
        SDL_Rect full = {0, 0, winW, winH};
        SDL_RenderFillRect(rnd, &full);

        if (fadeAlpha_ < 0.3f) {
            SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
            return;  // wait for fade before drawing text
        }

        float textAlpha = std::min(1.0f, (fadeAlpha_ - 0.3f) / 0.5f);
        int   cx = winW / 2;

        // ── WIN / LOSE headline ───────────────────────────────────────────
        if (data_.won) {
            // Pulsing gold/green for WIN
            float pulse = 0.85f + sinf(animTimer_ * 3.0f) * 0.15f;
            SDL_Color winCol = {
                (uint8_t)(100 * pulse),
                (uint8_t)(255 * pulse),
                (uint8_t)(80  * pulse),
                (uint8_t)(255 * textAlpha)
            };
            text.drawCentered("EXCELLENT!", cx, winH / 2 - 110, winCol, 32);
        } else {
            SDL_Color loseCol = {
                255, 80, 80,
                (uint8_t)(255 * textAlpha)
            };
            text.drawCentered("WRONG FISH...", cx, winH / 2 - 110, loseCol, 32);
        }

        // ── Divider line ──────────────────────────────────────────────────
        SDL_SetRenderDrawColor(rnd, 0, 229, 255, (uint8_t)(80 * textAlpha));
        SDL_RenderDrawLine(rnd, cx - 180, winH/2 - 70,
                                cx + 180, winH/2 - 70);

        // ── Stats rows ────────────────────────────────────────────────────
        SDL_Color cyan  = {0,   229, 255, (uint8_t)(255 * textAlpha)};
        SDL_Color amber = {255, 179, 0,   (uint8_t)(255 * textAlpha)};
        SDL_Color grey  = {140, 140, 180, (uint8_t)(255 * textAlpha)};

        int rowY = winH / 2 - 50;
        const int ROW_H = 32;

        // Difficulty
        drawStatRow(rnd, text, cx, rowY,
                    "DIFFICULTY", std::to_string(data_.difficulty),
                    grey, amber, (uint8_t)(255 * textAlpha), winW);
        rowY += ROW_H;

        // Time (only meaningful on win)
        if (data_.won) {
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "%.1f SEC", data_.timeTaken);
            drawStatRow(rnd, text, cx, rowY,
                        "TIME", timeBuf,
                        grey, cyan, (uint8_t)(255 * textAlpha), winW);
            rowY += ROW_H;
        }

        // Streak
        if (data_.streak > 0) {
            std::string streakStr = std::to_string(data_.streak) + " WIN STREAK";
            SDL_Color streakCol = {255, 215, 0, (uint8_t)(255 * textAlpha)};
            text.drawCentered(streakStr, cx, rowY, streakCol, 16);
        } else {
            SDL_Color streakCol = {140, 140, 180, (uint8_t)(255 * textAlpha)};
            text.drawCentered("STREAK RESET", cx, rowY, streakCol, 14);
        }
        rowY += ROW_H;

        // Win / Loss record
        char recordBuf[64];
        snprintf(recordBuf, sizeof(recordBuf),
                 "W %d  /  L %d", data_.totalWins, data_.totalLosses);
        text.drawCentered(recordBuf, cx, rowY, grey, 13);
        rowY += ROW_H + 8;

        // ── Currency placeholder (Phase 8 will fill this) ─────────────────
        SDL_SetRenderDrawColor(rnd, 0, 229, 255, (uint8_t)(25 * textAlpha));
        SDL_Rect rewardBox = {cx - 160, rowY, 320, 36};
        SDL_RenderFillRect(rnd, &rewardBox);
        SDL_SetRenderDrawColor(rnd, 0, 229, 255, (uint8_t)(50 * textAlpha));
        SDL_RenderDrawRect(rnd, &rewardBox);
        SDL_Color dimCyan = {0, 229, 255, (uint8_t)(80 * textAlpha)};
        text.drawCentered("REWARDS  ( PHASE 8 )", cx, rowY + 10, dimCyan, 11);
        rowY += 44 + 12;

        // ── Play Again button ─────────────────────────────────────────────
        SDL_Rect btn = buttonRect(winW, winH);
        SDL_Color btnBg   = btnHovered_
            ? SDL_Color{0, 229, 255, (uint8_t)(80  * textAlpha)}
            : SDL_Color{0, 229, 255, (uint8_t)(35  * textAlpha)};
        SDL_Color btnBord = btnHovered_
            ? SDL_Color{0, 229, 255, (uint8_t)(255 * textAlpha)}
            : SDL_Color{0, 229, 255, (uint8_t)(140 * textAlpha)};

        SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(rnd, btnBg.r, btnBg.g, btnBg.b, btnBg.a);
        SDL_RenderFillRect(rnd, &btn);
        SDL_SetRenderDrawColor(rnd, btnBord.r, btnBord.g, btnBord.b, btnBord.a);
        SDL_RenderDrawRect(rnd, &btn);

        const char* btnLabel = data_.won ? "PLAY AGAIN" : "TRY AGAIN";
        SDL_Color btnText = {0, 229, 255, (uint8_t)(255 * textAlpha)};
        text.drawCentered(btnLabel, btn.x + btn.w/2, btn.y + btn.h/2,
                          btnText, 16);

        SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
    }

private:
    ResultData data_;
    float animTimer_  = 0.0f;
    float fadeAlpha_  = 0.0f;
    bool  btnHovered_ = false;

    SDL_Rect buttonRect(int winW, int winH) const {
        return {winW/2 - 110, winH/2 + 130, 220, 44};
    }

    void drawStatRow(SDL_Renderer* rnd, TextRenderer& text,
                     int cx, int y,
                     const std::string& label, const std::string& value,
                     SDL_Color labelCol, SDL_Color valueCol,
                     uint8_t alpha, int winW) {
        (void)rnd; (void)winW; (void)alpha;
        // Label left, value right, both centered on cx with 120px gap
        text.draw(label, cx - 190, y, labelCol, 13);
        text.drawRight(value, cx + 190, y, valueCol, 13);
    }
};
