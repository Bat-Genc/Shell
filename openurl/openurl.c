// openurl.c
// Build for Windows: x86_64-w64-mingw32-gcc -O2 -o openurl.exe openurl.c
// Build for Linux:   cosmocc -O2 -o openurl openurl.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

#ifdef _WIN32
  #include <windows.h>

  int main(void) {
      HINSTANCE result = ShellExecuteA(NULL, "open", URL, NULL, NULL, SW_SHOWNORMAL);
      if ((INT_PTR)result <= 32) {
          fprintf(stderr, "Failed to open browser (error %ld).\n", (long)(INT_PTR)result);
          return 1;
      }
      fprintf(stderr, "Opened: %s\n", URL);
      return 0;
  }

#else

  int main(void) {
      char cmd[512];
    #ifdef __APPLE__
      snprintf(cmd, sizeof(cmd), "open '%s'", URL);
    #elif defined(__linux__)
      snprintf(cmd, sizeof(cmd), "xdg-open '%s'", URL);
    #else
      snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", URL);
    #endif
      fprintf(stderr, "Launching: %s\n", cmd);
      int rc = system(cmd);
      if (rc != 0) {
          fprintf(stderr, "Failed to open browser (exit %d).\n", rc);
          return 1;
      }
      return 0;
  }

#endif