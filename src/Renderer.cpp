#include <SDL2/SDL_video.h>
#include <cstdlib>
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

static void* checkAlloc (void *ptr)
{
  if (!ptr) {
    fprintf(stderr, "Fatal error: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }
  return ptr;
}

static const char* utf8ToCodepoint (const char *p, unsigned *dst)
{
  unsigned res, n;
  switch (*p & 0xf0) {
    case 0xf0 :  res = *p & 0x07;  n = 3;  break;
    case 0xe0 :  res = *p & 0x0f;  n = 2;  break;
    case 0xd0 :
    case 0xc0 :  res = *p & 0x1f;  n = 1;  break;
    default   :  res = *p;         n = 0;  break;
  }
  while (n--) {
    res = (res << 6) | (*(++p) & 0x3f);
  }
  *dst = res;
  return p + 1;
}


/// gets surface size
void RGetSize(int *x, int *y)
{
  SDL_Surface *surf = SDL_GetWindowSurface(window);
  *x = surf->w;
  *y = surf->h;
}

/// update all the rects in the window
void RUpdateRects(RRect *rects, int count)
{
  SDL_UpdateWindowSurfaceRects(window, (SDL_Rect*) rects, count);
  static bool initFrame = true;
  if (initFrame) {
    SDL_ShowWindow(window);
    initFrame = false;
  }
}

RImage* RNewImage (int w, int h)
{
  /// win dims need to be positive
  assert(w > 0 && h > 0);
  RImage *image = (RImage*) malloc (sizeof(RImage) + w * h * sizeof(RColor));
  checkAlloc(image);
  image->pixels = (RColor*) (image + 1);
  image->width = w;
  image->height = h;
  return image;
}


RImage* RFreeImage(RImage *image)
{
  free (image);
}