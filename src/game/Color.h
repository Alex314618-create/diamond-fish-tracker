#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdio>

// Parse a hex color string like "#FF6B6B" or "#FF6B6B99" into SDL_Color.
// Alpha defaults to 255 if not provided.
inline SDL_Color hexToSDL(const std::string& hex) {
    SDL_Color c = {0, 0, 0, 255};
    if (hex.size() < 7 || hex[0] != '#') return c;
    auto b = [&](int pos) {
        return (uint8_t)std::stoul(hex.substr(pos, 2), nullptr, 16);
    };
    c.r = b(1); c.g = b(3); c.b = b(5);
    if (hex.size() >= 9) c.a = b(7);
    return c;
}

// Darken a hex color by 30% (matches JS Sprites.darken())
inline std::string darkenHex(const std::string& hex) {
    if (hex.size() < 7 || hex[0] != '#') return hex;
    auto b = [&](int pos) {
        return (int)std::stoul(hex.substr(pos, 2), nullptr, 16);
    };
    auto fmt = [](int v) -> std::string {
        v = std::clamp((int)(v * 0.7f), 0, 255);
        char buf[4];
        snprintf(buf, sizeof(buf), "%02x", v);
        return std::string(buf);
    };
    return "#" + fmt(b(1)) + fmt(b(3)) + fmt(b(5));
}

// Set SDL renderer draw color from hex string + optional alpha override
inline void setDrawColor(SDL_Renderer* r, const std::string& hex, uint8_t alpha = 255) {
    SDL_Color c = hexToSDL(hex);
    c.a = alpha;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

// Draw a filled rectangle (mirrors Canvas fillRect)
inline void fillRect(SDL_Renderer* r, int x, int y, int w, int h) {
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(r, &rect);
}
