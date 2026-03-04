#pragma once
#include <SDL2/SDL.h>
#include <cmath>
#include <string>
#include <vector>
#include "game/Color.h"

// ---- Fish type enum ----
enum class FishType { Tropical, Puffer, Angel, Shark, Goldfish, Jelly, COUNT };

inline const std::vector<std::string>& fishColors(FishType t) {
    static const std::vector<std::string> palettes[(int)FishType::COUNT] = {
        {"#FF6B6B","#4ECDC4","#FFE66D","#C594FF"},  // Tropical
        {"#FFA07A","#FFB6C1","#87CEEB","#F5DEB3"},  // Puffer
        {"#E0E0E0","#FFD700","#B0C4DE","#DDA0DD"},  // Angel
        {"#708090","#4169E1","#483D8B","#F8F8FF"},  // Shark
        {"#FF8C00","#FF4500","#FFFAF0","#FFD700"},  // Goldfish
        {"#DA70D6","#00CED1","#98FB98","#F0FFFF"},  // Jelly
    };
    return palettes[(int)t];
}

inline FishType randomFishType() {
    return (FishType)(rand() % (int)FishType::COUNT);
}

inline std::string randomFishColor(FishType t) {
    const auto& p = fishColors(t);
    return p[rand() % p.size()];
}

// Draw a single fish sprite centered at pixel (cx, cy).
// dir: +1 = facing right, -1 = facing left
// frame: incrementing float driving sine animations
// color: hex string e.g. "#FF6B6B"
// turnScale: horizontal squash [0..1] for turn animation
inline void drawFish(SDL_Renderer* rnd,
                     FishType type, int cx, int cy,
                     float frame, int dir,
                     const std::string& color,
                     float turnScale = 1.0f)
{
    // FR(dx, dy, w, h):
    //   Draws a filled rect offset (dx, dy) from fish center (cx, cy).
    //   When dir == -1, the x-axis is mirrored: dx is negated and the rect
    //   shifts left by w so it stays on the correct side.
    //   turnScale squashes the horizontal extent around the center.
    //   Color must be set by the CALLER via setDrawColor() before calling FR().
    auto FR = [&](int dx, int dy, int w, int h) {
        int rx, rw;
        if (dir < 0) {
            // Mirror: the rect that was at +dx now sits at -(dx+w)
            rx = cx + (int)((-dx - w) * turnScale);
        } else {
            rx = cx + (int)(dx * turnScale);
        }
        rw = std::max(1, (int)(w * turnScale));
        SDL_Rect rect = { rx, cy + dy, rw, h };
        SDL_RenderFillRect(rnd, &rect);
    };

    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_BLEND);

    switch (type) {

    case FishType::Tropical: {
        // Body
        setDrawColor(rnd, color);
        FR(-12, -5, 24, 10);
        FR( -8, -7, 16, 14);
        // Stripes
        setDrawColor(rnd, darkenHex(color));
        FR(-6, -7, 3, 14);
        FR( 2, -7, 3, 14);
        // Tail (animated)
        int tw = (int)(sinf(frame * 0.3f) * 2);
        setDrawColor(rnd, color);
        FR(-16 + tw, -4, 4,  8);
        FR(-20 + tw, -6, 4, 12);
        // Fin (animated)
        int fw = (int)(sinf(frame * 0.4f) * 2);
        FR(-2, -11 + fw, 6, 4);
        // Eye
        setDrawColor(rnd, "#FFFFFF");
        FR(6, -3, 4, 4);
        setDrawColor(rnd, "#000000");
        FR(8, -1, 2, 2);
        break;
    }

    case FishType::Puffer: {
        setDrawColor(rnd, color);
        FR( -9,  -9, 18, 18);
        FR(-11,  -7, 22, 14);
        FR( -7, -11, 14, 22);
        // Spots
        setDrawColor(rnd, darkenHex(color));
        FR(-5, -5, 3, 3);
        FR( 2,  1, 3, 3);
        FR(-3,  4, 3, 3);
        // Fins (animated)
        int pf = (int)(sinf(frame * 0.5f) * 2);
        setDrawColor(rnd, color);
        FR(-13, -1 + pf, 3, 3);
        FR( 10, -1 - pf, 3, 3);
        // Tail
        FR(-15, -3, 3, 6);
        // Eye
        setDrawColor(rnd, "#FFFFFF");
        FR(3, -5, 5, 5);
        setDrawColor(rnd, "#000000");
        FR(6, -3, 2, 2);
        break;
    }

    case FishType::Angel: {
        setDrawColor(rnd, color);
        FR(-6,  -7, 12, 14);
        FR(-4, -11,  8, 22);
        FR(-2, -13,  4, 26);
        // Stripe
        setDrawColor(rnd, darkenHex(color));
        FR(-1, -11, 2, 22);
        // Fins (animated)
        int af = (int)(sinf(frame * 0.3f) * 2);
        setDrawColor(rnd, color);
        FR(-1, -17 + af, 3, 5);
        FR(-1,  12 - af, 3, 5);
        // Tail (animated)
        int at = (int)(sinf(frame * 0.4f) * 2);
        FR(-10 + at, -5, 4, 10);
        // Eye
        setDrawColor(rnd, "#FFFFFF");
        FR(2, -4, 4, 4);
        setDrawColor(rnd, "#000000");
        FR(4, -2, 2, 2);
        break;
    }

    case FishType::Shark: {
        setDrawColor(rnd, color);
        FR(-14, -5, 28, 10);
        FR(-10, -7, 20, 14);
        // Belly
        setDrawColor(rnd, "#DDDDDD");
        FR(-10, 2, 20, 4);
        // Nose
        setDrawColor(rnd, color);
        FR(14, -3, 4, 6);
        FR(18, -1, 3, 2);
        // Dorsal fin (animated)
        int sf = (int)(sinf(frame * 0.3f));
        FR(-3, -11 + sf, 6, 5);
        FR(-1, -13 + sf, 3, 2);
        // Tail (animated)
        int st = (int)(sinf(frame * 0.4f) * 3);
        FR(-18 + st, -7, 4, 5);
        FR(-22 + st, -9, 4, 3);
        FR(-18 + st,  2, 4, 5);
        FR(-22 + st,  6, 4, 3);
        // Eye
        setDrawColor(rnd, "#FFFFFF");
        FR(6, -4, 4, 4);
        setDrawColor(rnd, "#000000");
        FR(8, -2, 2, 2);
        break;
    }

    case FishType::Goldfish: {
        setDrawColor(rnd, color);
        FR(-8, -6, 16, 12);
        FR(-6, -8, 12, 16);
        // Flowing tail (animated)
        int gt  = (int)(sinf(frame * 0.3f) * 3);
        FR(-12 + gt,             -5,  4, 10);
        FR(-16 + (int)(gt*1.2f), -7,  4, 14);
        FR(-20 + (int)(gt*1.5f), -9,  4, 18);
        // Top fin (animated)
        int gf = (int)(sinf(frame * 0.4f) * 2);
        FR(-4, -12 + gf, 10, 4);
        FR(-2, -14 + gf,  6, 2);
        // Side fins
        FR( 0,  7 - gf, 5, 4);
        FR(-7,  7 + gf, 5, 4);
        // Eye
        setDrawColor(rnd, "#FFFFFF");
        FR(4, -4, 4, 4);
        setDrawColor(rnd, "#000000");
        FR(6, -2, 2, 2);
        break;
    }

    case FishType::Jelly: {
        SDL_Color jc = hexToSDL(color);
        // Bell — semi-transparent (alpha 0x99 = 153)
        SDL_SetRenderDrawColor(rnd, jc.r, jc.g, jc.b, 153);
        FR( -9, -14, 18, 10);
        FR(-11, -10, 22,  6);
        FR( -7, -16, 14,  4);
        // Inner glow
        SDL_SetRenderDrawColor(rnd, 255, 255, 255, 85);
        FR(-5, -12, 10, 6);
        // Pulse (animated)
        int jp = (int)(sinf(frame * 0.1f) * 2);
        SDL_SetRenderDrawColor(rnd, jc.r, jc.g, jc.b, 119);
        FR(-9 - jp, -4, 18 + jp * 2, 3);
        // Tentacles
        SDL_SetRenderDrawColor(rnd, jc.r, jc.g, jc.b, 102);
        for (int i = 0; i < 5; i++) {
            int off = (int)(sinf(frame * 0.2f + i) * 4);
            int tx  = -7 + (int)(i * 3.5f);
            FR(tx + off,              -1, 2, 6);
            FR(tx + (int)(off*1.2f),   5, 2, 6);
            FR(tx + (int)(off*1.4f),  11, 2, 5);
            if (i % 2 == 0)
                FR(tx + (int)(off*1.6f), 16, 2, 4);
        }
        break;
    }

    default: break;
    }

    SDL_SetRenderDrawBlendMode(rnd, SDL_BLENDMODE_NONE);
}
