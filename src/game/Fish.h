#pragma once
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include "game/Sprites.h"

enum class Personality { Lazy, Hyper, Social, Explorer, Zigzag, COUNT };

struct Bounds {
    float left, right, top, bottom;
};

class Fish {
public:
    float x, y;
    float vx, vy;
    int   dir;          // +1 right, -1 left
    float frame;
    FishType    type;
    std::string color;
    Personality personality;

    // Game-state flags
    bool  hasDiamond        = false;
    bool  isSwallowing      = false;
    int   swallowFrame      = 0;
    bool  revealing         = false;
    float turnProgress      = 1.0f;
    int   targetDir         = 1;
    float speedMultiplier   = 1.0f;
    float diamondSpeedBoost = 1.0f;
    bool  isGrayscale       = false;
    // Spread phase fields (Phase 3)
    struct SpreadPos { float x = 0, y = 0; } spreadTarget;
    bool  isSpreading       = false;

    Fish(float startX, float startY, FishType t, const std::string& col)
        : x(startX), y(startY), type(t), color(col)
    {
        vx = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
        vy = ((float)rand() / RAND_MAX - 0.5f) * 1.0f;
        dir = vx > 0 ? 1 : -1;
        targetDir = dir;
        frame = (float)(rand() % 100);
        personality = (Personality)(rand() % (int)Personality::COUNT);
    }

    // dt: milliseconds since last frame
    void update(std::vector<Fish>& fishes, const Bounds& bounds, float dt) {
        frame += dt * 0.06f;

        // --- Personality movement (exact copy of HTML logic) ---
        switch (personality) {
        case Personality::Lazy:
            vx *= 0.98f; vy *= 0.98f;
            if ((float)rand()/RAND_MAX < 0.02f) {
                vx += ((float)rand()/RAND_MAX - 0.5f) * 0.3f;
                vy += ((float)rand()/RAND_MAX - 0.5f) * 0.2f;
            }
            break;
        case Personality::Hyper: {
            if ((float)rand()/RAND_MAX < 0.05f) {
                vx += ((float)rand()/RAND_MAX - 0.5f) * 1.5f;
                vy += ((float)rand()/RAND_MAX - 0.5f) * 1.0f;
            }
            float spd = sqrtf(vx*vx + vy*vy);
            if (spd > 3.5f) { vx = vx/spd*3.5f; vy = vy/spd*3.5f; }
            break;
        }
        case Personality::Social: {
            Fish* nearest = nullptr;
            float minD = 1e9f;
            for (auto& f : fishes) {
                if (&f == this) continue;
                float d = hypotf(f.x - x, f.y - y);
                if (d < minD && d > 40.0f) { minD = d; nearest = &f; }
            }
            if (nearest && minD < 150.0f) {
                vx += (nearest->x - x) * 0.001f;
                vy += (nearest->y - y) * 0.001f;
            }
            break;
        }
        case Personality::Explorer:
            if (x > bounds.right  - 80) vx -= 0.04f;
            if (x < bounds.left   + 80) vx += 0.04f;
            if (y > bounds.bottom - 80) vy -= 0.04f;
            if (y < bounds.top    + 80) vy += 0.04f;
            if ((float)rand()/RAND_MAX < 0.02f) {
                vx += ((float)rand()/RAND_MAX - 0.5f) * 0.8f;
                vy += ((float)rand()/RAND_MAX - 0.5f) * 0.4f;
            }
            break;
        case Personality::Zigzag:
            if ((int)(frame / 25) % 2 == 0) vy += 0.015f;
            else                              vy -= 0.015f;
            if ((float)rand()/RAND_MAX < 0.01f)
                vx = ((float)rand()/RAND_MAX - 0.5f) * 2.5f;
            break;
        default: break;
        }

        // --- Global damping ---
        vx *= 0.995f;
        vy *= 0.995f;
        if (fabsf(vx) < 0.1f && fabsf(vy) < 0.1f) {
            vx += ((float)rand()/RAND_MAX - 0.5f) * 0.4f;
            vy += ((float)rand()/RAND_MAX - 0.5f) * 0.2f;
        }

        // --- Repulsion (personal space = 45px) ---
        const float personalSpace = 45.0f;
        float repelX = 0, repelY = 0;
        for (auto& other : fishes) {
            if (&other == this) continue;
            float dx = x - other.x;
            float dy = y - other.y;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < personalSpace && dist > 0) {
                float force = (personalSpace - dist) / personalSpace;
                repelX += (dx / dist) * force * 1.5f;
                repelY += (dy / dist) * force * 1.5f;
            }
        }
        vx += repelX * 0.2f;
        vy += repelY * 0.2f;

        // --- Apply speed ---
        float speedMod = speedMultiplier;
        if (hasDiamond) speedMod *= diamondSpeedBoost;
        x += vx * speedMod;
        y += vy * speedMod;

        // --- Wall bounce ---
        if (x < bounds.left   + 25) { x = bounds.left   + 25; vx *= -0.8f; }
        if (x > bounds.right  - 25) { x = bounds.right  - 25; vx *= -0.8f; }
        if (y < bounds.top    + 25) { y = bounds.top    + 25; vy *= -0.8f; }
        if (y > bounds.bottom - 25) { y = bounds.bottom - 25; vy *= -0.8f; }

        // --- Turn animation ---
        if (fabsf(vx) > 0.1f) targetDir = vx > 0 ? 1 : -1;
        if (dir != targetDir) {
            turnProgress -= 0.08f;
            if (turnProgress <= 0) { dir = targetDir; turnProgress = 1.0f; }
        } else {
            turnProgress = std::min(1.0f, turnProgress + 0.08f);
        }

        // --- Swallow animation ---
        if (isSwallowing) {
            swallowFrame++;
            if (swallowFrame > 25) isSwallowing = false;
        }
    }

    void draw(SDL_Renderer* rnd) const {
        // Turn squash: horizontal scale = |cos(turnProgress * PI)|
        float scaleX = (turnProgress < 1.0f)
            ? fabsf(cosf(turnProgress * 3.14159f))
            : 1.0f;

        drawFish(rnd, type, (int)x, (int)y, frame, dir, color, scaleX);

        // Swallow flash (cyan square shrinking)
        if (isSwallowing) {
            float sz = std::max(0.0f, 7.0f - swallowFrame * 0.3f);
            setDrawColor(rnd, "#00FFFF");
            SDL_Rect flash = {
                (int)(x + dir * 8 - sz/2),
                (int)(y - sz/2),
                (int)sz, (int)sz
            };
            SDL_RenderFillRect(rnd, &flash);
        }

        // Diamond reveal halo (pulsing cyan circle - approximated with rects)
        if (revealing && hasDiamond) {
            SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);
            float alpha = 0.3f + sinf(frame * 0.2f) * 0.2f;
            SDL_Color hc = hexToSDL("#00FFFF");
            SDL_SetRenderDrawColor(rnd, hc.r, hc.g, hc.b, (uint8_t)(alpha * 255));
            for (int r = 20; r <= 25; r++) {
                SDL_Rect ring = { (int)x - r, (int)y - r, r*2, r*2 };
                SDL_RenderDrawRect(rnd, &ring);
            }
            SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
        }
    }

    // Hit test: matches HTML Fish.contains()
    bool contains(float px, float py) const {
        return fabsf(px - x) < 20.0f && fabsf(py - y) < 18.0f;
    }
};
