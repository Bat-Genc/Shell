// openurl.c — portable browser launcher
//
// Build (Cosmopolitan — works on all platforms):
//   cosmocc -O2 -o openurl.com openurl.c
//
// Build for Windows only (from Linux):
//   x86_64-w64-mingw32-gcc -O2 -o openurl.exe openurl.c
//
// Build for Linux only:
//   gcc -O2 -o openurl openurl.c
//
// Run:
//   ./openurl
//   ./openurl https://example.com
//   openurl.exe https://example.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __COSMOPOLITAN__
  #include <cosmo.h>   // IsWindows(), IsLinux(), IsXnu(), etc.
#endif

#define DEFAULT_URL "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

int main(int argc, char *argv[]) {
    const char *url = (argc > 1) ? argv[1] : DEFAULT_URL;
    char cmd[1024];

    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr,
                "Usage: %s [URL]\n"
                "Opens URL in the default browser.\n"
                "If no URL is given, opens the default one:\n"
                "  %s\n",
                argv[0], DEFAULT_URL);
        return 0;
    }

#ifdef __COSMOPOLITAN__
    // ===== RUNTIME detection (works because of Cosmopolitan) =====
    if (IsWindows()) {
        snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", url);
    } else if (IsXnu()) {
        snprintf(cmd, sizeof(cmd), "open '%s'", url);
    } else if (IsLinux()) {
        snprintf(cmd, sizeof(cmd), "xdg-open '%s'", url);
    } else if (IsFreebsd() || IsOpenbsd() || IsNetbsd()) {
        snprintf(cmd, sizeof(cmd),
                 "xdg-open '%s' 2>/dev/null || "
                 "(which open >/dev/null 2>&1 && open '%s')",
                 url, url);
    } else {
        snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", url);
    }
#else
    // ===== COMPILE-TIME detection (for gcc / mingw / clang) =====
  #if defined(_WIN32)
    snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", url);
  #elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open '%s'", url);
  #elif defined(__linux__)
    snprintf(cmd, sizeof(cmd), "xdg-open '%s'", url);
  #else
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", url);
  #endif
#endif

    fprintf(stderr, "Launching: %s\n", cmd);

    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Failed to open browser (exit %d).\n", rc);
        return 1;
    }

    return 0;
}