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
    window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, kScreenWidth,
                              kScreenHeight, SDL_WINDOW_SHOWN);
    if (window == NULL) {
      LogSDLError("Window could not be created! SDL_Error: %s");
    } else {
      // Get window surface
      screenSurface = SDL_GetWindowSurface(window);
      psxSystem = SystemNew(biosPath, NULL, NULL);
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
    SystemRun(psxSystem);
    SDL_FillRect(screenSurface, NULL,
                 SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

    SDL_UpdateWindowSurface(window);
  }
}

void LogSDLError(const char *format) {
  PCFStringRef errorStr = PCFStringNewFromCString(SDL_GetError());
  PCFERROR(format, SDL_GetError());
  PCFRelease(errorStr);
}
