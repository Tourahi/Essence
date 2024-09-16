#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include "lib/stb/stb_truetype.h"
#include "include/Renderer.hpp"


/// Window
static SDL_Window *window;

struct RImage {
  RColor *pixels;
  int width, height;
};

#define GLYPHSET_MAX 256

typedef struct {
  RImage *image;
  stbtt_bakedchar glyphs[GLYPHSET_MAX];
} GlyphSet;

struct RFont {
  void* data;
  stbtt_fontinfo stbfont;
  float size;
  int height;
  GlyphSet *sets[GLYPHSET_MAX];
};

static struct { int left, top, right, bottom; } clip;

/// sets the window clip
void RSetClipRect (RRect rect)
{
  clip.left = rect.x;
  clip.top = rect.y;
  clip.right = rect.x + rect.width;
  clip.bottom = rect.y + rect.height;
}

void RInit(SDL_Window *win)
{
  assert(win);
  window = win;
  SDL_Surface *surf = SDL_GetWindowSurface(window);
  RSetClipRect( (RRect) {0, 0, surf->w, surf->h} );
}
