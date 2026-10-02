// openurl.c
// Build:  cosmocc -O2 -o openurl openurl.c
// Run:    ./openurl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libc/dce.h"   // <--- ADD THIS LINE

#include "libc/dce.h"   // Cosmopolitan: IsWindows(), IsXnu(), IsLinux(), IsFreebsd(), etc.

#define URL "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

int main(void) {
    char cmd[512];

    if (IsWindows()) {
        // Windows: `start` is a cmd.exe builtin. The empty "" is the window title
        // argument so that a quoted URL isn't misinterpreted as the title.
        // Cosmo maps this to the system shell, so `start` works.
        snprintf(cmd, sizeof(cmd), "start \"\" \"%s\"", URL);

    } else if (IsXnu()) {
        // macOS (XNU kernel)
        snprintf(cmd, sizeof(cmd), "open '%s'", URL);

    } else if (IsLinux()) {
        // Most Linux desktops
        snprintf(cmd, sizeof(cmd), "xdg-open '%s'", URL);

    } else if (IsFreebsd() || IsOpenbsd() || IsNetbsd()) {
        // BSDs — try xdg-open first, fall back to the older `open` shim
        snprintf(cmd, sizeof(cmd),
                 "xdg-open '%s' 2>/dev/null || "
                 "(which open >/dev/null 2>&1 && open '%s')",
                 URL, URL);

    } else {
        // Unknown platform — best-effort fallback
        snprintf(cmd, sizeof(cmd), "xdg-open '%s' 2>/dev/null", URL);
    }

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
