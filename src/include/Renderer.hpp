#pragma once

#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct RImage RImage;
typedef struct RFont RFont;

typedef struct { uint8_t r, g, b, a; } RColor;
typedef struct { int x, y, width, height; } RRect;

/// Init SDL window
/// It does not create the window it should be provided
void RInit (SDL_Window *win);

void RSetClipRect (RRect rect);
void RGetSize (int *x, int *y);
void RUpdateRects (RRect *rects, int count);

/// RImage creation
RImage* RNewImage(int w, int h);
void RFreeImage(RImage *image);

/// RFont
RFont* RLoadFont (const char *filename, float size);
void RFreeFont (RFont *font);
void RSetFontTabWidth (RFont *font, int w);
int RGetFontTabWidth (RFont *font);
int RGetFontWidth (RFont *font, const char *text);
int RGetFontHeigh (RFont *font);

/// Drawing

void RDrawRect (RRect rect, RColor color);
void RDrawImage (RImage *image, RRect *sub, int x, int y, RColor color);
void RDrawText (RFont *font, const char *text, int x, int y, RColor color);