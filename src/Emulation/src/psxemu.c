// Using SDL and standard IO
#include "System.h"
#include <PsxCoreFoundation/String.h>
#include <SDL.h>
#include <SDL_main.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <wchar.h>

// Screen dimension constants
const int kScreenWidth = 640;
const int kScreenHeight = 480;
const char *kBiosPath = "..\\..\\..\\..\\..\\SCPH1001.BIN";
const char *kWindowTitle = "PsxEmu";

void LogSDLError(const char *format);
bool Init(PCFStringRef _Nonnull biosPath);
void Loop();
void Close();

SDL_Window *window = NULL;
SDL_Surface *screenSurface = NULL;
System *psxSystem = NULL;

int main(int argc, char *args[]) {
  setlocale(LC_ALL, "C.UTF-8");
  PCFStringRef biosPath = PCFCSTR(kBiosPath);
  if (Init(biosPath)) {
    Loop();
  }
  Close();

  return 0;
}

void Close() {
  // Destroy window
  SDL_DestroyWindow(window);

  // Quit SDL subsystems
  SDL_Quit();
}

bool Init(PCFStringRef _Nonnull biosPath) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LogSDLError("SDL could not initialize! SDL_Error: %s");
  } else {
    // Create window
    window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kScreenWidth,
                              kScreenHeight, SDL_WINDOW_SHOWN);
    if (window == NULL) {
      LogSDLError("Window could not be created! SDL_Error: %s");
    } else {
      psxSystem = SystemNew(biosPath, NULL, NULL);
      screenSurface = SDL_GetWindowSurface(window);
      PCFStringRef pixelName = PCFStringNewFromCString(SDL_GetPixelFormatName(screenSurface->format->format));
      PCFDEBUG("Pixel format is %s. Num of bytes per pixel is %d. R = 0x%08x, "
               "G = 0x%08x, B = 0x%08x",
               pixelName, screenSurface->format->BytesPerPixel, screenSurface->format->Rmask,
               screenSurface->format->Gmask, screenSurface->format->Bmask);
      PCFRelease(pixelName);
      return true;
    }
  }
  return false;
}

void Loop() {
  bool quit = false;
  SDL_Event e;
  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }
    screenSurface = SDL_GetWindowSurface(window);
    SystemRun(psxSystem);
    SystemUpdateSurface(psxSystem, screenSurface);
    SDL_UpdateWindowSurface(window);
    SystemSync(psxSystem);
  }
}

void LogSDLError(const char *format) {
  PCFStringRef errorStr = PCFStringNewFromCString(SDL_GetError());
  PCFERROR(format, SDL_GetError());
  PCFRelease(errorStr);
}
