#pragma once
// Minimal SDL2 subset for the Emscripten (browser) build.
//
// The browser build runs the firmware's main() on a worker thread
// (PROXY_TO_PTHREAD), where the real SDL2 port cannot own the DOM. This header
// keeps the simulator's HAL sources unchanged: rendering lands in a shared
// RGBA staging buffer that the page blits to a <canvas>, and keyboard/mouse
// events arrive through an exported queue fed by the page. Only the calls the
// simulator uses are provided. Native builds keep using the real SDL2.
#ifndef __EMSCRIPTEN__
#error "web/SDL.h is only for Emscripten builds"
#endif

#include <cstdint>

typedef uint8_t Uint8;
typedef uint32_t Uint32;
typedef int32_t Sint32;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

struct SDL_Rect {
  int x, y, w, h;
};

struct SDL_Surface {
  int w, h, pitch;
  void *pixels;
};

typedef enum { SDL_FLIP_NONE = 0 } SDL_RendererFlip;

typedef enum {
  SDL_SCANCODE_UNKNOWN = 0,
  SDL_SCANCODE_H = 11,
  SDL_SCANCODE_P = 19,
  SDL_SCANCODE_S = 22,
  SDL_SCANCODE_RETURN = 40,
  SDL_SCANCODE_ESCAPE = 41,
  SDL_SCANCODE_RIGHT = 79,
  SDL_SCANCODE_LEFT = 80,
  SDL_SCANCODE_DOWN = 81,
  SDL_SCANCODE_UP = 82,
  SDL_NUM_SCANCODES = 512
} SDL_Scancode;

enum {
  SDL_QUIT = 0x100,
  SDL_KEYDOWN = 0x300,
  SDL_KEYUP = 0x301,
  SDL_MOUSEMOTION = 0x400,
  SDL_MOUSEBUTTONDOWN = 0x401,
  SDL_MOUSEBUTTONUP = 0x402
};

enum { SDL_BUTTON_LEFT = 1 };
enum { SDL_INIT_VIDEO = 0x20 };
enum { SDL_WINDOWPOS_UNDEFINED = 0x1FFF0000 };
enum { SDL_WINDOW_SHOWN = 0x4, SDL_WINDOW_ALLOW_HIGHDPI = 0x2000 };
enum { SDL_RENDERER_ACCELERATED = 0x2 };
enum { SDL_TEXTUREACCESS_STREAMING = 1 };
enum { SDL_PIXELFORMAT_ARGB8888 = 0x16362004 };
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"

struct SDL_Keysym {
  SDL_Scancode scancode;
};
struct SDL_KeyboardEvent {
  Uint32 type;
  Uint8 repeat;
  SDL_Keysym keysym;
};
struct SDL_MouseButtonEvent {
  Uint32 type;
  Uint8 button;
  Sint32 x, y;
};
struct SDL_MouseMotionEvent {
  Uint32 type;
  Sint32 x, y;
};
union SDL_Event {
  Uint32 type;
  SDL_KeyboardEvent key;
  SDL_MouseButtonEvent button;
  SDL_MouseMotionEvent motion;
};

int SDL_Init(Uint32 flags);
void SDL_Quit();
const char *SDL_GetError();
Uint32 SDL_GetTicks();
void SDL_Delay(Uint32 ms);
int SDL_SetHint(const char *name, const char *value);

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h,
                             Uint32 flags);
void SDL_SetWindowSize(SDL_Window *window, int w, int h);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags);
int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h);
int SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h);
SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format,
                               int access, int w, int h);
int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect,
                      const void *pixels, int pitch);
int SDL_RenderClear(SDL_Renderer *renderer);
int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture,
                   const SDL_Rect *src, const SDL_Rect *dst);
int SDL_RenderCopyEx(SDL_Renderer *renderer, SDL_Texture *texture,
                     const SDL_Rect *src, const SDL_Rect *dst, double angle,
                     const void *center, SDL_RendererFlip flip);
void SDL_RenderPresent(SDL_Renderer *renderer);
int SDL_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *rect,
                         Uint32 format, void *pixels, int pitch);
SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int w, int h,
                                                int depth, int pitch,
                                                Uint32 format);
int SDL_SaveBMP(SDL_Surface *surface, const char *file);
void SDL_FreeSurface(SDL_Surface *surface);

int SDL_PollEvent(SDL_Event *event);
const Uint8 *SDL_GetKeyboardState(int *numkeys);
