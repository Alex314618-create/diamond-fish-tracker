#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include "game/Color.h"

// Pixel-art diamond gem, drawn centered at (x,y).
// Matches the HTML Diamond object's visual style.
inline void drawDiamond(SDL_Renderer* rnd, float x, float y, float frame) {
    int cx = (int)x, cy = (int)y;

    // Color cycles: cyan -> white -> gold
    float t = fmodf(frame * 0.04f, 3.0f);
    SDL_Color c;
    if (t < 1.0f) {
        // cyan -> white
        uint8_t v = (uint8_t)(t * 255);
        c = {v, 255, 255, 255};
    } else if (t < 2.0f) {
        // white -> gold
        float s = t - 1.0f;
        c = {255, 255, (uint8_t)(255 - s * 200), 255};
    } else {
        // gold -> cyan
        float s = t - 2.0f;
        c = {(uint8_t)(255 - s * 255), (uint8_t)(200 + s * 55), (uint8_t)(s * 255), 255};
    }

    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);

    // Glow halo
    SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, 40);
    for (int r = 18; r <= 22; r++) {
        SDL_Rect ring = {cx - r, cy - r, r*2, r*2};
        SDL_RenderDrawRect(rnd, &ring);
    }

    // Diamond body (pixel-art gem shape)
    SDL_SetRenderDrawColor(rnd, c.r, c.g, c.b, 255);
    // Top facets
    fillRect(rnd, cx-2, cy-8, 4, 2);
    fillRect(rnd, cx-4, cy-6, 8, 2);
    // Wide middle
    fillRect(rnd, cx-6, cy-4, 12, 2);
    fillRect(rnd, cx-7, cy-2, 14, 3);
    fillRect(rnd, cx-6, cy+1, 12, 2);
    // Lower facets
    fillRect(rnd, cx-4, cy+3, 8, 2);
    fillRect(rnd, cx-2, cy+5, 4, 2);
    // Tip
    fillRect(rnd, cx-1, cy+7, 2, 2);

    // Inner highlight (white)
    SDL_SetRenderDrawColor(rnd, 255, 255, 255, 180);
    fillRect(rnd, cx-2, cy-5, 3, 4);
    fillRect(rnd, cx-1, cy-4, 2, 3);

    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
}

struct DiamondDrop {
    float x = 0, y = 0;
    float startX = 0, startY = 0;
    float targetX = 0, targetY = 0;
    float frame = 0;
    bool  falling = false;
    bool  visible = false;
    int   targetFishIdx = -1;

    void reset(float tx, float ty) {
        targetX = tx;
        targetY = ty;
        startX  = tx;
        startY  = ty - 200.0f;   // start above the fish
        x = startX;
        y = startY;
        falling = true;
        visible = true;
        frame   = 0;
    }

    void hide() { visible = false; falling = false; }

    // Returns true when the diamond has reached the target fish
    bool update() {
        frame += 1.0f;
        if (!falling || !visible) return false;
        float dy = targetY - y;
        if (fabsf(dy) < 6.0f) {
            x = targetX;
            y = targetY;
            falling = false;
            return true;   // signal: arrived
        }
        y += dy * 0.12f;   // ease-in toward target
        x += (targetX - x) * 0.12f;
        return false;
    }

    void draw(SDL_Renderer* rnd) const {
        if (!visible) return;
        drawDiamond(rnd, x, y, frame);
    }
};
