#pragma once

#if defined(__APPLE__)
#include "MD5Builder_mac.h"
#elif defined(__linux__)
#include "MD5Builder_linux.h"
#elif defined(__EMSCRIPTEN__)
#include "web/MD5Builder_web.h"
#else
#error "Unsupported host OS for simulator MD5Builder"
#endif
