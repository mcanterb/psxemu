// Using SDL and standard IO
#include <PsxCoreFoundation/Data.h>
#include <PsxCoreFoundation/String.h>
#include <SDL.h>
#include <SDL_main.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <wchar.h>

// Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main(int argc, char *args[]) {
  setlocale(LC_ALL, "C.UTF-8");

  // The window we'll be rendering to
  SDL_Window *window = NULL;

  // The surface contained by the window
  SDL_Surface *screenSurface = NULL;

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
  } else {
    // Create window
    window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
      printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    } else {
      // Get window surface
      screenSurface = SDL_GetWindowSurface(window);

      // Fill the surface white
      SDL_FillRect(screenSurface, NULL,
                   SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

      // Update the surface
      SDL_UpdateWindowSurface(window);

      PCFDataRef data = PCFDataResultOrPanic(PCFDataNewFromFile(
          PCFCSTR("D:\\Users\\Matthew\\projects\\psxemu\\SCPH1001.BIN")));
      int i;
      for (i = 0; i < 10000000; i++) {
        PCFDEBUG("Showing window! Test = %o", data);
      }
      PCFRelease(data);

      // Wait two seconds
      SDL_Delay(2000);
    }
  }
  // Destroy window
  SDL_DestroyWindow(window);

  // Quit SDL subsystems
  SDL_Quit();

  return 0;
}
