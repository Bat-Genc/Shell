// openurl.c — portable browser launcher
//
// Build:
//   Windows (from Linux): x86_64-w64-mingw32-gcc -O2 -o openurl.exe openurl.c
//   Linux:                gcc -O2 -o openurl openurl.c
//   macOS:                clang -O2 -o openurl openurl.c
//   Cosmopolitan:         cosmocc -O2 -o openurl.elf openurl.c
//
// Run:
//   ./openurl
//   ./openurl https://example.com
//   openurl.exe https://example.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default URL if no argument is given
#define DEFAULT_URL "https://www.youtube.com/watch?v=a8tUtAJeHVg"

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

#if defined(_WIN32)
    snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", url);
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open '%s'", url);
#elif defined(__linux__)
    snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", url);
#else
    snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", url);
#endif

    fprintf(stderr, "Launching: %s\n", cmd);

    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Failed to open browser (exit %d).\n", rc);
        return 1;
    }

    return 0;
}