// Browser platform layer for the Emscripten build (see web/SDL.h).
//
// Threading model: the firmware's main() runs on a worker (PROXY_TO_PTHREAD).
// The page's main thread only calls the exported sim_web_* functions: it
// pushes input into a single-producer/single-consumer queue and polls the
// published RGBA frame through a sequence counter. Everything else stays in
// the unchanged simulator HAL sources.
#ifdef __EMSCRIPTEN__

#include "SDL.h"

#include <emscripten.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

struct SDL_Window {
  int w = 0, h = 0;
};

struct SDL_Texture {
  int w = 0, h = 0;
  std::vector<uint32_t> argb;
};

struct SDL_Renderer {
  int w = 0, h = 0;
  std::vector<uint32_t> argb; // current back buffer, logical size
};

namespace {

double startMs = emscripten_get_now();
Uint8 keyboardState[SDL_NUM_SCANCODES] = {};

// Input queue: page main thread produces, firmware thread consumes.
constexpr uint32_t kQueueSize = 256;
SDL_Event queue[kQueueSize];
std::atomic<uint32_t> queueHead{0};
std::atomic<uint32_t> queueTail{0};

void pushEvent(const SDL_Event &event) {
  const uint32_t head = queueHead.load(std::memory_order_relaxed);
  const uint32_t next = (head + 1) % kQueueSize;
  if (next == queueTail.load(std::memory_order_acquire))
    return; // full: drop, like a busy HID buffer
  queue[head] = event;
  queueHead.store(next, std::memory_order_release);
}

// Published frame (RGBA8888, row major) guarded by a seqlock: odd while
// writing, even when stable. The page copies only when the value is even and
// unchanged across the copy.
std::mutex publishMutex;
std::vector<uint8_t> published;
std::atomic<uint32_t> publishedSeq{0};
std::atomic<int> publishedW{0};
std::atomic<int> publishedH{0};

void rotatePoint(int angle, double x, double y, double &ox, double &oy) {
  switch (((angle % 360) + 360) % 360) {
  case 90:
    ox = -y;
    oy = x;
    break;
  case 180:
    ox = -x;
    oy = -y;
    break;
  case 270:
    ox = y;
    oy = -x;
    break;
  default:
    ox = x;
    oy = y;
  }
}

// Draws `texture` into `dst` rotated clockwise by `angle` degrees (multiples
// of 90) around the centre of `dst`, like SDL_RenderCopyEx.
void blit(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *dstRect,
          int angle) {
  if (!r || !t || r->w <= 0 || r->h <= 0)
    return;
  const SDL_Rect dst = dstRect ? *dstRect : SDL_Rect{0, 0, r->w, r->h};
  if (dst.w <= 0 || dst.h <= 0)
    return;
  const double cx = dst.x + dst.w / 2.0;
  const double cy = dst.y + dst.h / 2.0;
  for (int y = 0; y < r->h; ++y) {
    for (int x = 0; x < r->w; ++x) {
      // Inverse-rotate the output pixel centre back into dst space.
      double qx = 0, qy = 0;
      rotatePoint(-angle, x + 0.5 - cx, y + 0.5 - cy, qx, qy);
      qx += cx;
      qy += cy;
      if (qx < dst.x || qy < dst.y || qx >= dst.x + dst.w ||
          qy >= dst.y + dst.h)
        continue;
      const int tx = static_cast<int>((qx - dst.x) * t->w / dst.w);
      const int ty = static_cast<int>((qy - dst.y) * t->h / dst.h);
      if (tx < 0 || ty < 0 || tx >= t->w || ty >= t->h)
        continue;
      r->argb[static_cast<size_t>(y) * r->w + x] =
          t->argb[static_cast<size_t>(ty) * t->w + tx];
    }
  }
}

void resize(SDL_Renderer *r, int w, int h) {
  if (!r || w <= 0 || h <= 0 || (r->w == w && r->h == h))
    return;
  r->w = w;
  r->h = h;
  r->argb.assign(static_cast<size_t>(w) * h, 0xFF000000u);
}

SDL_Renderer *activeRenderer = nullptr;

} // namespace

int SDL_Init(Uint32) { return 0; }
void SDL_Quit() {}
const char *SDL_GetError() { return "unsupported in browser build"; }
Uint32 SDL_GetTicks() {
  return static_cast<Uint32>(emscripten_get_now() - startMs);
}
void SDL_Delay(Uint32 ms) { usleep(static_cast<useconds_t>(ms) * 1000); }
int SDL_SetHint(const char *, const char *) { return 1; }

SDL_Window *SDL_CreateWindow(const char *, int, int, int w, int h, Uint32) {
  auto *window = new SDL_Window();
  window->w = w;
  window->h = h;
  return window;
}

void SDL_SetWindowSize(SDL_Window *window, int w, int h) {
  if (window) {
    window->w = w;
    window->h = h;
  }
  resize(activeRenderer, w, h);
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int, Uint32) {
  auto *renderer = new SDL_Renderer();
  if (window)
    resize(renderer, window->w, window->h);
  activeRenderer = renderer;
  return renderer;
}

int SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) {
  resize(renderer, w, h);
  return 0;
}

int SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h) {
  if (!renderer)
    return -1;
  if (w)
    *w = renderer->w;
  if (h)
    *h = renderer->h;
  return 0;
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *, Uint32, int, int w, int h) {
  auto *texture = new SDL_Texture();
  texture->w = w;
  texture->h = h;
  texture->argb.assign(static_cast<size_t>(w) * h, 0xFFFFFFFFu);
  return texture;
}

int SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *, const void *pixels,
                      int pitch) {
  if (!texture || !pixels)
    return -1;
  const auto *src = static_cast<const uint8_t *>(pixels);
  for (int y = 0; y < texture->h; ++y)
    std::memcpy(&texture->argb[static_cast<size_t>(y) * texture->w],
                src + static_cast<size_t>(y) * pitch,
                static_cast<size_t>(texture->w) * sizeof(uint32_t));
  return 0;
}

int SDL_RenderClear(SDL_Renderer *renderer) {
  if (renderer)
    std::fill(renderer->argb.begin(), renderer->argb.end(), 0xFF000000u);
  return 0;
}

int SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture,
                   const SDL_Rect *, const SDL_Rect *dst) {
  blit(renderer, texture, dst, 0);
  return 0;
}

int SDL_RenderCopyEx(SDL_Renderer *renderer, SDL_Texture *texture,
                     const SDL_Rect *, const SDL_Rect *dst, double angle,
                     const void *, SDL_RendererFlip) {
  blit(renderer, texture, dst, static_cast<int>(angle));
  return 0;
}

void SDL_RenderPresent(SDL_Renderer *renderer) {
  if (!renderer || renderer->w <= 0 || renderer->h <= 0)
    return;
  std::lock_guard<std::mutex> lock(publishMutex);
  publishedSeq.fetch_add(1, std::memory_order_acq_rel); // odd: writing
  published.resize(static_cast<size_t>(renderer->w) * renderer->h * 4);
  for (size_t i = 0; i < renderer->argb.size(); ++i) {
    const uint32_t p = renderer->argb[i];
    published[i * 4 + 0] = static_cast<uint8_t>(p >> 16);
    published[i * 4 + 1] = static_cast<uint8_t>(p >> 8);
    published[i * 4 + 2] = static_cast<uint8_t>(p);
    published[i * 4 + 3] = 0xFF;
  }
  publishedW.store(renderer->w, std::memory_order_relaxed);
  publishedH.store(renderer->h, std::memory_order_relaxed);
  publishedSeq.fetch_add(1, std::memory_order_acq_rel); // even: stable
}

int SDL_RenderReadPixels(SDL_Renderer *renderer, const SDL_Rect *, Uint32,
                         void *pixels, int pitch) {
  if (!renderer || !pixels)
    return -1;
  auto *dst = static_cast<uint8_t *>(pixels);
  for (int y = 0; y < renderer->h; ++y)
    std::memcpy(dst + static_cast<size_t>(y) * pitch,
                &renderer->argb[static_cast<size_t>(y) * renderer->w],
                static_cast<size_t>(renderer->w) * sizeof(uint32_t));
  return 0;
}

SDL_Surface *SDL_CreateRGBSurfaceWithFormatFrom(void *pixels, int w, int h,
                                                int, int pitch, Uint32) {
  auto *surface = new SDL_Surface();
  surface->w = w;
  surface->h = h;
  surface->pitch = pitch;
  surface->pixels = pixels;
  return surface;
}

int SDL_SaveBMP(SDL_Surface *, const char *) { return -1; }
void SDL_FreeSurface(SDL_Surface *surface) { delete surface; }

int SDL_PollEvent(SDL_Event *event) {
  const uint32_t tail = queueTail.load(std::memory_order_relaxed);
  if (tail == queueHead.load(std::memory_order_acquire))
    return 0;
  if (event)
    *event = queue[tail];
  queueTail.store((tail + 1) % kQueueSize, std::memory_order_release);
  return 1;
}

const Uint8 *SDL_GetKeyboardState(int *numkeys) {
  if (numkeys)
    *numkeys = SDL_NUM_SCANCODES;
  return keyboardState;
}

// ---- Exports called from the page's main thread ----

extern "C" {

EMSCRIPTEN_KEEPALIVE void sim_web_key(int scancode, int down) {
  if (scancode <= 0 || scancode >= SDL_NUM_SCANCODES)
    return;
  const bool wasDown = keyboardState[scancode] != 0;
  keyboardState[scancode] = down ? 1 : 0;
  SDL_Event event{};
  event.key.type = down ? SDL_KEYDOWN : SDL_KEYUP;
  event.key.repeat = (down && wasDown) ? 1 : 0;
  event.key.keysym.scancode = static_cast<SDL_Scancode>(scancode);
  pushEvent(event);
}

EMSCRIPTEN_KEEPALIVE void sim_web_mouse(int type, int x, int y) {
  SDL_Event event{};
  if (type == 0) {
    event.button.type = SDL_MOUSEBUTTONDOWN;
  } else if (type == 1) {
    event.motion.type = SDL_MOUSEMOTION;
    event.motion.x = x;
    event.motion.y = y;
    pushEvent(event);
    return;
  } else {
    event.button.type = SDL_MOUSEBUTTONUP;
  }
  event.button.button = SDL_BUTTON_LEFT;
  event.button.x = x;
  event.button.y = y;
  pushEvent(event);
}

EMSCRIPTEN_KEEPALIVE void sim_web_quit() {
  SDL_Event event{};
  event.type = SDL_QUIT;
  pushEvent(event);
}

EMSCRIPTEN_KEEPALIVE uint32_t sim_web_frame_seq() {
  return publishedSeq.load(std::memory_order_acquire);
}
EMSCRIPTEN_KEEPALIVE const uint8_t *sim_web_frame_ptr() {
  return published.data();
}
EMSCRIPTEN_KEEPALIVE int sim_web_frame_width() {
  return publishedW.load(std::memory_order_relaxed);
}
EMSCRIPTEN_KEEPALIVE int sim_web_frame_height() {
  return publishedH.load(std::memory_order_relaxed);
}

} // extern "C"

#endif // __EMSCRIPTEN__
