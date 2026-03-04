#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <iostream>

// ============================================================
// FONT CONFIGURATION - change this one line to swap fonts
// Must match an exact filename in assets/fonts/
// ============================================================
constexpr const char* GAME_FONT_FILE = "PressStart2P-Regular.ttf";

// Size presets
constexpr int FONT_SIZE_LARGE  = 20;   // overlay banners
constexpr int FONT_SIZE_MEDIUM = 14;   // HUD labels
constexpr int FONT_SIZE_SMALL  = 11;   // small indicators

// ============================================================
// TextRenderer
// Usage:
//   TextRenderer tr;
//   tr.init(renderer, "assets/fonts");
//   tr.drawCentered("HELLO", 400, 300, {0,229,255,255}, FONT_SIZE_LARGE);
//   tr.destroy();
// ============================================================
class TextRenderer {
public:
    bool init(SDL_Renderer* rnd, const std::string& fontsDir) {
        rnd_ = rnd;

        if (TTF_Init() < 0) {
            std::cerr << "[TextRenderer] TTF_Init: " << TTF_GetError() << "\n";
            return false;
        }

        // Build full path: fontsDir + "/" + GAME_FONT_FILE
        std::string path = fontsDir + "/" + GAME_FONT_FILE;

        fontLarge_  = TTF_OpenFont(path.c_str(), FONT_SIZE_LARGE);
        fontMedium_ = TTF_OpenFont(path.c_str(), FONT_SIZE_MEDIUM);
        fontSmall_  = TTF_OpenFont(path.c_str(), FONT_SIZE_SMALL);

        if (!fontLarge_) {
            std::cerr << "[TextRenderer] Failed to load: " << path << "\n";
            std::cerr << "  TTF error: " << TTF_GetError() << "\n";
            std::cerr << "  Run Step 1 again and verify assets/fonts/ contents.\n";
            return false;
        }

        std::cout << "[TextRenderer] Font loaded: " << path << "\n";
        return true;
    }

    void destroy() {
        if (fontLarge_)  { TTF_CloseFont(fontLarge_);  fontLarge_  = nullptr; }
        if (fontMedium_) { TTF_CloseFont(fontMedium_); fontMedium_ = nullptr; }
        if (fontSmall_)  { TTF_CloseFont(fontSmall_);  fontSmall_  = nullptr; }
        TTF_Quit();
    }

    // Draw text with top-left at (x, y)
    void draw(const std::string& txt, int x, int y,
              SDL_Color color, int size = FONT_SIZE_MEDIUM) {
        if (txt.empty()) return;
        TTF_Font* f = pickFont(size);
        if (!f) return;
        renderText(txt, x, y, color, f);
    }

    // Draw text centered horizontally and vertically at (cx, cy)
    void drawCentered(const std::string& txt, int cx, int cy,
                      SDL_Color color, int size = FONT_SIZE_LARGE) {
        if (txt.empty()) return;
        TTF_Font* f = pickFont(size);
        if (!f) return;
        int w = 0, h = 0;
        TTF_SizeText(f, txt.c_str(), &w, &h);
        renderText(txt, cx - w / 2, cy - h / 2, color, f);
    }

    // Draw text right-aligned so its right edge is at (rx, y)
    void drawRight(const std::string& txt, int rx, int y,
                   SDL_Color color, int size = FONT_SIZE_SMALL) {
        if (txt.empty()) return;
        TTF_Font* f = pickFont(size);
        if (!f) return;
        int w = 0, h = 0;
        TTF_SizeText(f, txt.c_str(), &w, &h);
        renderText(txt, rx - w, y, color, f);
    }

    // Returns text width in pixels for the given size
    int measureWidth(const std::string& txt, int size = FONT_SIZE_MEDIUM) {
        TTF_Font* f = pickFont(size);
        if (!f) return 0;
        int w = 0, h = 0;
        TTF_SizeText(f, txt.c_str(), &w, &h);
        return w;
    }

private:
    SDL_Renderer* rnd_        = nullptr;
    TTF_Font*     fontLarge_  = nullptr;
    TTF_Font*     fontMedium_ = nullptr;
    TTF_Font*     fontSmall_  = nullptr;

    TTF_Font* pickFont(int size) const {
        if (size >= 18) return fontLarge_;
        if (size >= 12) return fontMedium_;
        return fontSmall_;
    }

    void renderText(const std::string& txt, int x, int y,
                    SDL_Color color, TTF_Font* font) {
        SDL_Surface* surf = TTF_RenderText_Blended(font, txt.c_str(), color);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(rnd_, surf);
        if (tex) {
            SDL_Rect dst = { x, y, surf->w, surf->h };
            SDL_RenderCopy(rnd_, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        }
        SDL_FreeSurface(surf);
    }
};
