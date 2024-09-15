#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct RImage RImage;
typedef struct RFont RFont;

typedef struct { uint8_t b, g, r, a; } RColor;
typedef struct { int x, y, width, height; } RRect;

/// Init SDL window
/// It does not create the window it should be provided
void RInit (SDL_Window *win);

void RSetClipRect (RRect rect);
