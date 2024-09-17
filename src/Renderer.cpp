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
#define INIT_IMAGE_WIDTH 128
#define INIT_IMAGE_HEIGHT 128

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


void RFreeImage(RImage *image)
{
  free (image);
}

static GlyphSet* loadGlyphset(RFont* font, int idx)
{
  GlyphSet *set = (GlyphSet*) checkAlloc(calloc(1, sizeof(GlyphSet)));

  // image init
  int width  = INIT_IMAGE_WIDTH;
  int height = INIT_IMAGE_HEIGHT;

  bool validBufferSize = false;


  while (!validBufferSize)
  {

    set->image = RNewImage(width, height);

    // basically doing this "pixels / (ascent - descent)" but fancy :).
    float s = 
      stbtt_ScaleForMappingEmToPixels(&font->stbfont, 1) /
      stbtt_ScaleForPixelHeight(&font->stbfont, 1);

    int res = stbtt_BakeFontBitmap((const unsigned char*) font->data, 0, font->size * s, 
      (unsigned char*) set->image->pixels,
      width, height, idx * 256, 256, set->glyphs);

    // if size is not enough DOUBLE ITTTT.
    if (res < 0) {
      width *= 2;
      width *= 2;
      RFreeImage(set->image);
      continue;
    }
    validBufferSize = true;
  }

  int asc, desc, linegap;
  stbtt_GetFontVMetrics(&font->stbfont, &asc, &desc, &linegap);

  float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, font->size);
  int scaledAsc = asc * scale * 0.5;

  for (int i = 0; i < 256; i++) {
    /* align glyphs properly with the baseline */
    set->glyphs[i].yoff += scaledAsc;
    /* ensure integer values for pixel-perfect rendering && to remove fractional spacing */
    set->glyphs[i].xadvance = floor(set->glyphs[i].xadvance);
  }

  /* convert 8bit data to 32bit */
  for (int i = width * height - 1; i >= 0; i--) {
    /* cast to uint8_t ptr and then offset it by i */
    uint8_t n = *((uint8_t*) set->image->pixels + i);
    set->image->pixels[i] = (RColor){ .r = 255, .g = 255, .b = 255, .a = n};
  }

  return set;
}

static GlyphSet* getGlyphset (RFont *font, int codepoint)
{
  int idx = (codepoint >> 8) % GLYPHSET_MAX;
  if (!font->sets[idx]) {
    font->sets[idx] = loadGlyphset(font, idx);
  }
  return font->sets[idx];
}


RFont* RLoadFont (const char *filename, float size)
{
  RFont *font = NULL;
  FILE *fp = NULL;

  font = (RFont*) checkAlloc(calloc(1, sizeof(RFont)));
  font->size = size;

  fp = fopen(filename, "rb");
  if (!fp) { return NULL; }

  fseek(fp, 0, SEEK_END);
  int buffSize = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  /// load
  font->data = checkAlloc(malloc(buffSize));
  int _ = fread(font->data, 1, buffSize, fp);
  (void) _;
  fclose(fp);
  fp = NULL;

  /// init stbfont
  int ok = stbtt_InitFont(&font->stbfont, (const unsigned char*) font->data, 0);
  if (!ok) {
    if (fp) { fclose(fp); }
    if (font) { free(font->data); }
    free (font);
    return NULL;
  } else {
    /// get height and scale
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&font->stbfont, &ascent, &descent, &linegap);
    float scale = stbtt_ScaleForMappingEmToPixels(&font->stbfont, size);
    font->height = (ascent - descent + linegap) * scale + 0.5;
    
    /// make tab and newline glyphs invisible
    stbtt_bakedchar *g = getGlyphset(font, '\n')->glyphs;
    g['\t'].x1 = g['\t'].x0;
    g['\n'].x1 = g['\n'].x0;

    return font;
  }
}

void RFreeFont(RFont *font)
{
  for (int i = 0; i < GLYPHSET_MAX; i++) {
    GlyphSet *set = font->sets[i];
    if (set) {
      RFreeImage(set->image);
      free (set);
    }
    free (font->data);
    free (font);
  }
}

void RSetFontTabWidth(RFont *font, int w)
{
  GlyphSet *set = getGlyphset(font, '\t');
  set->glyphs['\t'].xadvance = n;
}

int RGetFontTabWidth(RFont *font)
{
  GlyphSet *set = getGlyphset(font, '\t');
  return set->glyphs['\t'].xadvance;
}

int RGetFontHeigh(RFont *font)
{
  return font->height;
}

int RGetFontWidth(RFont *font, const char *text)
{
  int x = 0;
  const char *p = text;
  unsigned codepoint;
  while (*p) {
    p = utf8ToCodepoint(p, &codepoint);
    stbtt_bakedchar *g = &set->glyphs[codepoint & 0xff];
    x += g->xadvance;
  }
  return x;
}

