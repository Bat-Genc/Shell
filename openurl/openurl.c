// openurl.c
// Build:  cosmocc -O2 -o openurl openurl.c
// Run:    ./openurl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

int main(void) {
    char cmd[512];

#if defined(_WIN32)
    snprintf(cmd, sizeof(cmd), "cmd.exe /c start \"\" \"%s\"", URL);
#elif defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "open '%s'", URL);
#elif defined(__linux__)
    snprintf(cmd, sizeof(cmd), "xdg-open '%s'", URL);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    snprintf(cmd, sizeof(cmd),
             "xdg-open '%s' 2>/dev/null || "
             "(which open >/dev/null 2>&1 && open '%s')",
             URL, URL);
#else
    snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", URL);
#endif

    fprintf(stderr, "Launching: %s\n", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr,
                "Failed to open browser (exit %d).\n"
                "Make sure a default browser is configured.\n", rc);
        return 1;
    }
    return 0;
}